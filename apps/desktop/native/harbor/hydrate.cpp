#include "http.h"

#include <map>
#include <memory>
#include <thread>

using namespace winrt;

namespace harbor {

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
    int64_t end = fetch.offset + fetch.length - 1;
    HttpStream stream(L"/u/" + fileIdOf(fetch.identity),
                      L"Range: bytes=" + std::to_wstring(fetch.offset) + L"-" + std::to_wstring(end) + L"\r\n");

    if (stream.status != 200 && stream.status != 206) {
        throw std::runtime_error("HTTP " + std::to_string(stream.status));
    }

    int64_t skip = stream.status == 200 ? fetch.offset : 0;
    int64_t position = fetch.offset;
    int64_t remaining = fetch.length;
    std::vector<char> chunk;
    chunk.reserve(CHUNK_SIZE);
    std::vector<char> buffer(64 * 1024);

    while (remaining > 0) {
        if (*fetch.isCancelled) return;

        DWORD read = stream.read(buffer.data(), static_cast<DWORD>(buffer.size()));
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

std::string contentOf(std::wstring const& identity) {
    if (identity.starts_with(L"l:")) {
        return toUtf8(L"[InternetShortcut]\r\nURL=" + config().instanceUrl + L"/s/" + identity.substr(2) + L"\r\n");
    }

    if (!identity.starts_with(L"n:")) throw std::runtime_error("Not a note or a link");

    auto response = httpGet(L"/api/notes/" + identity.substr(2));
    if (response.status != 200) throw std::runtime_error("HTTP " + std::to_string(response.status));

    auto note = JsonObject::Parse(toWide(std::string(response.body.begin(), response.body.end())));
    return toUtf8(std::wstring(note.GetNamedString(L"content")));
}

void serveContent(Fetch const& fetch) {
    auto content = contentOf(fetch.identity);

    if (static_cast<int64_t>(content.size()) != fetch.fileSize) {
        log(L"Download failed: the content of " + fetch.identity + L" does not match its placeholder");
        fail(fetch);

        if (fetch.identity.starts_with(L"n:")) {
            JsonObject message;
            message.SetNamedValue(L"identity", JsonValue::CreateStringValue(fetch.identity));
            sendEvent(L"noteChanged", message);
        }
        return;
    }

    transfer(fetch, STATUS_SUCCESS, content.data() + fetch.offset, fetch.offset, fetch.length);
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
            if (fetch.identity.starts_with(L"f:")) download(fetch);
            else serveContent(fetch);
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
