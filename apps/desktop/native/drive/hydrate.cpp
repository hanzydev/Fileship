#include "common.h"

#include <winhttp.h>

#include <map>
#include <memory>
#include <thread>

using namespace winrt;

namespace drive {

namespace {

constexpr int64_t CHUNK_SIZE = 1024 * 1024;

struct Fetch {
    CF_CONNECTION_KEY connectionKey;
    CF_TRANSFER_KEY transferKey;
    CF_REQUEST_KEY requestKey;
    std::wstring identity;
    int64_t offset;
    int64_t length;
    int64_t fileSize;
    std::shared_ptr<std::atomic<bool>> isCancelled;
};

std::mutex fetchMutex;
std::map<int64_t, std::shared_ptr<std::atomic<bool>>> activeFetches;

HINTERNET session() {
    static HINTERNET handle = [] {
        HINTERNET opened = WinHttpOpen(config().userAgent.c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                       WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        WinHttpSetTimeouts(opened, 15000, 15000, 30000, 60000);
        return opened;
    }();

    return handle;
}

struct Internet {
    HINTERNET value = nullptr;
    explicit Internet(HINTERNET handle) : value(handle) {}
    ~Internet() {
        if (value) WinHttpCloseHandle(value);
    }
};

void transfer(Fetch const& fetch, NTSTATUS status, void const* buffer, int64_t offset, int64_t length) {
    CF_OPERATION_INFO info{};
    info.StructSize = sizeof(info);
    info.Type = CF_OPERATION_TYPE_TRANSFER_DATA;
    info.ConnectionKey = fetch.connectionKey;
    info.TransferKey = fetch.transferKey;
    info.RequestKey = fetch.requestKey;

    CF_OPERATION_PARAMETERS parameters{};
    parameters.ParamSize = CF_SIZE_OF_OP_PARAM(TransferData);
    parameters.TransferData.CompletionStatus = status;
    parameters.TransferData.Buffer = buffer;
    parameters.TransferData.Offset.QuadPart = offset;
    parameters.TransferData.Length.QuadPart = length;

    check_hresult(CfExecute(&info, &parameters));
}

void fail(Fetch const& fetch) {
    try {
        transfer(fetch, STATUS_UNSUCCESSFUL, nullptr, fetch.offset, fetch.length);
    } catch (hresult_error const& error) {
        log(L"Could not report a failed download: " + std::wstring(error.message()));
    }
}

std::wstring fileIdOf(std::wstring const& identity) {
    if (!identity.starts_with(L"f:")) throw std::runtime_error("Not a file");
    return identity.substr(2);
}

void download(Fetch fetch) {
    URL_COMPONENTS url{};
    url.dwStructSize = sizeof(url);
    wchar_t host[256]{};
    wchar_t path[2048]{};
    url.lpszHostName = host;
    url.dwHostNameLength = static_cast<DWORD>(std::size(host));
    url.lpszUrlPath = path;
    url.dwUrlPathLength = static_cast<DWORD>(std::size(path));

    std::wstring base = config().instanceUrl;
    if (!WinHttpCrackUrl(base.c_str(), 0, 0, &url)) throw_last_error();

    std::wstring requestPath = std::wstring(path);
    while (!requestPath.empty() && requestPath.back() == L'/') requestPath.pop_back();
    requestPath += L"/u/" + fileIdOf(fetch.identity);

    Internet connection(WinHttpConnect(session(), host, url.nPort, 0));
    if (!connection.value) throw_last_error();

    DWORD flags = url.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    Internet request(WinHttpOpenRequest(connection.value, L"GET", requestPath.c_str(), nullptr,
                                        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags));
    if (!request.value) throw_last_error();

    int64_t end = fetch.offset + fetch.length - 1;
    std::wstring headers = L"Authorization: Bearer " + token() + L"\r\nRange: bytes=" +
                           std::to_wstring(fetch.offset) + L"-" + std::to_wstring(end) + L"\r\n";

    if (!WinHttpSendRequest(request.value, headers.c_str(), static_cast<DWORD>(headers.size()),
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(request.value, nullptr)) {
        throw_last_error();
    }

    DWORD status = 0;
    DWORD size = sizeof(status);
    WinHttpQueryHeaders(request.value, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);

    if (status != 200 && status != 206) throw std::runtime_error("HTTP " + std::to_string(status));

    int64_t skip = status == 200 ? fetch.offset : 0;
    int64_t position = fetch.offset;
    int64_t remaining = fetch.length;
    std::vector<char> chunk;
    chunk.reserve(CHUNK_SIZE);
    std::vector<char> buffer(64 * 1024);

    while (remaining > 0) {
        if (*fetch.isCancelled) return;

        DWORD read = 0;
        if (!WinHttpReadData(request.value, buffer.data(), static_cast<DWORD>(buffer.size()), &read)) {
            throw_last_error();
        }
        if (read == 0) break;

        int64_t start = 0;
        if (skip > 0) {
            start = std::min<int64_t>(skip, read);
            skip -= start;
        }

        int64_t usable = std::min<int64_t>(static_cast<int64_t>(read) - start, remaining - static_cast<int64_t>(chunk.size()));
        chunk.insert(chunk.end(), buffer.data() + start, buffer.data() + start + usable);

        if (static_cast<int64_t>(chunk.size()) >= CHUNK_SIZE || static_cast<int64_t>(chunk.size()) == remaining) {
            transfer(fetch, STATUS_SUCCESS, chunk.data(), position, static_cast<int64_t>(chunk.size()));
            position += static_cast<int64_t>(chunk.size());
            remaining -= static_cast<int64_t>(chunk.size());
            chunk.clear();

            LARGE_INTEGER total{};
            LARGE_INTEGER completed{};
            total.QuadPart = fetch.fileSize;
            completed.QuadPart = position;
            CfReportProviderProgress(fetch.connectionKey, fetch.transferKey, total, completed);
        }
    }

    if (remaining > 0) throw std::runtime_error("The download ended early");
}

}

void fetchData(CF_CALLBACK_INFO const* info, CF_CALLBACK_PARAMETERS const* parameters) {
    auto isCancelled = std::make_shared<std::atomic<bool>>(false);

    Fetch fetch{
        info->ConnectionKey,
        info->TransferKey,
        info->RequestKey,
        identityOf(info),
        parameters->FetchData.RequiredFileOffset.QuadPart,
        parameters->FetchData.RequiredLength.QuadPart,
        info->FileSize.QuadPart,
        isCancelled,
    };

    {
        std::lock_guard lock(fetchMutex);
        activeFetches[fetch.transferKey.QuadPart] = isCancelled;
    }

    std::thread([fetch] {
        try {
            download(fetch);
        } catch (hresult_error const& error) {
            log(L"Download failed: " + std::wstring(error.message()));
            fail(fetch);
        } catch (std::exception const& error) {
            log(L"Download failed: " + toWide(error.what()));
            fail(fetch);
        }

        std::lock_guard lock(fetchMutex);
        activeFetches.erase(fetch.transferKey.QuadPart);
    }).detach();
}

void cancelFetch(CF_CALLBACK_INFO const* info) {
    std::lock_guard lock(fetchMutex);
    auto found = activeFetches.find(info->TransferKey.QuadPart);
    if (found != activeFetches.end()) *found->second = true;
}

}
