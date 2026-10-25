#include "app.h"
#include "rpc.h"

#include <shared/log.h>

#include <bcrypt.h>

#include <atomic>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <fstream>
#include <mutex>
#include <thread>

namespace fileship::host {

namespace {

constexpr int MAX_ATTEMPTS = 3;
constexpr int MAX_CONCURRENT = 3;
constexpr int64_t LIMITS_TTL_MS = 5 * 60 * 1000;

struct Limits {
    int64_t chunkSize = 0;
    int concurrency = 1;
    int64_t at = 0;
};

struct ProgressItem {
    std::wstring id;
    std::wstring name;
    int progress = 0;
};

std::mutex mutex;
std::condition_variable slots;
std::deque<uint64_t> waiting;
uint64_t nextTicket = 0;
int running = 0;
std::vector<ProgressItem> progressItems;
std::optional<Limits> cachedLimits;

void publish() {
    broadcast(L"uploads:progress", uploadProgress());
    refreshTray();
}

void setProgress(std::wstring const& id, int value) {
    std::lock_guard lock(mutex);
    for (auto& item : progressItems) {
        if (item.id == id) item.progress = value;
    }
}

Limits uploadLimits() {
    {
        std::lock_guard lock(mutex);
        if (cachedLimits && nowMs() - cachedLimits->at < LIMITS_TTL_MS) return *cachedLimits;
    }

    std::optional<JsonObject> publicSettings;
    try {
        auto value = api(L"GET", L"/api/settings/public");
        if (value.ValueType() == JsonValueType::Object) publicSettings = value.GetObject();
    } catch (std::exception const&) {
    }

    Limits next{static_cast<int64_t>((publicSettings ? jsonNumber(*publicSettings, L"fileChunkSize", 25) : 25) * 1024 *
                                     1024),
                std::max(1, static_cast<int>(publicSettings ? jsonNumber(*publicSettings, L"uploadConcurrency", 1) : 1)),
                nowMs()};

    if (publicSettings) {
        std::lock_guard lock(mutex);
        cachedLimits = next;
    }

    return next;
}

std::string escapeFileName(std::wstring const& name) {
    std::string result;
    for (char character : toUtf8(name)) {
        if (character == '"') result += "%22";
        else if (character == '\r') result += "%0D";
        else if (character == '\n') result += "%0A";
        else result += character;
    }

    return result;
}

std::optional<JsonObject> sendChunk(std::function<Multipart()> const& form) {
    for (int attempt = 1;; attempt++) {
        try {
            auto body = form();
            auto response = apiForm(L"/api/files", body, 120000);
            if (response.ValueType() == JsonValueType::Object) return response.GetObject();
            return std::nullopt;
        } catch (ApiError const& error) {
            if (error.status == 401) handleUnauthorized();

            bool isRetryable = error.status == 0 || error.status >= 500;
            if (!isRetryable || attempt >= MAX_ATTEMPTS) throw;

            Sleep(static_cast<DWORD>(attempt * 1000));
        }
    }
}

UploadedFile runJob(UploadSource const& job, std::wstring const& id, UploadOptions const& options,
                    UploadProgress const& onProgress) {
    auto limits = uploadLimits();
    auto size = limits.chunkSize;

    std::vector<uint8_t> random(12);
    BCryptGenRandom(nullptr, random.data(), static_cast<ULONG>(random.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    auto uploadId = base64(random.data(), random.size());
    for (auto& character : uploadId) {
        if (character == L'+') character = L'-';
        if (character == L'/') character = L'_';
    }

    bool isFile = job.data.empty() && !job.path.empty();
    int64_t fileSize = isFile ? static_cast<int64_t>(std::filesystem::file_size(job.path))
                              : static_cast<int64_t>(job.data.size());
    int64_t totalChunks = std::max<int64_t>(1, (fileSize + size - 1) / size);

    std::mutex readMutex;
    std::ifstream file;
    if (isFile) {
        file.open(job.path, std::ios::binary);
        if (!file) throw translatedError(t(L"tasks.uploadFailed"));
    }

    auto readChunk = [&](int64_t offset) {
        auto length = static_cast<size_t>(std::min(size, fileSize - offset));
        if (!isFile) return std::vector<char>(job.data.begin() + offset, job.data.begin() + offset + length);

        std::lock_guard lock(readMutex);
        std::vector<char> buffer(length);
        file.clear();
        file.seekg(offset);
        file.read(buffer.data(), static_cast<std::streamsize>(length));
        if (file.gcount() != static_cast<std::streamsize>(length)) throw translatedError(t(L"tasks.uploadFailed"));
        return buffer;
    };

    std::mutex stateMutex;
    std::optional<JsonObject> result;
    std::atomic<int64_t> next = 0;
    int64_t sent = 0;
    std::exception_ptr failure;

    auto sendPart = [&](int64_t index) {
        auto offset = index * size;
        auto chunk = readChunk(offset);

        auto response = sendChunk([&] {
            Multipart form;
            form.field("uploadId", toUtf8(uploadId));
            form.field("currentChunk", std::to_string(index + 1));
            form.field("totalChunks", std::to_string(totalChunks));
            form.field("chunkOffset", std::to_string(offset));
            form.field("fileNameType", toUtf8(options.fileNameType));
            if (options.expiration) form.field("expiration", toUtf8(*options.expiration));
            if (options.maxViews) form.field("maxViews", std::to_string(*options.maxViews));
            if (options.compression) form.field("compression", std::to_string(*options.compression));
            if (options.password) form.field("password", toUtf8(*options.password));
            if (options.folderId) form.field("folderId", toUtf8(*options.folderId));
            form.file("file", escapeFileName(job.name), toUtf8(job.mimeType), chunk.data(), chunk.size());
            return form;
        });

        int64_t total = 0;
        {
            std::lock_guard lock(stateMutex);
            if (response) result = response;
            sent += static_cast<int64_t>(chunk.size());
            total = sent;
        }

        setProgress(id, fileSize ? static_cast<int>(std::lround(static_cast<double>(total) / fileSize * 100)) : 100);
        publish();
        if (onProgress) onProgress(total, fileSize);
    };

    auto worker = [&] {
        while (true) {
            {
                std::lock_guard lock(stateMutex);
                if (failure) return;
            }

            auto index = next++;
            if (index >= totalChunks) return;

            try {
                sendPart(index);
            } catch (...) {
                std::lock_guard lock(stateMutex);
                if (!failure) failure = std::current_exception();
            }
        }
    };

    std::vector<std::thread> workers;
    auto count = std::min<int64_t>(limits.concurrency, totalChunks);
    for (int64_t index = 1; index < count; index++) workers.emplace_back(worker);
    worker();
    for (auto& thread : workers) thread.join();

    if (failure) std::rethrow_exception(failure);
    if (!result || jsonString(*result, L"url").empty()) throw translatedError(t(L"tasks.noLink"));

    return UploadedFile{jsonString(*result, L"id"), jsonString(*result, L"name"), jsonString(*result, L"fileName"),
                        jsonString(*result, L"url"), jsonString(*result, L"directUrl")};
}

}

std::optional<UploadSummary> uploadSummary() {
    std::lock_guard lock(mutex);
    if (progressItems.empty()) return std::nullopt;

    double sum = 0;
    for (auto const& item : progressItems) sum += item.progress;
    return UploadSummary{progressItems.size(), static_cast<int>(std::lround(sum / progressItems.size()))};
}

JsonArray uploadProgress() {
    std::lock_guard lock(mutex);
    JsonArray result;

    for (auto const& item : progressItems) {
        JsonObject entry;
        entry.SetNamedValue(L"id", JsonValue::CreateStringValue(item.id));
        entry.SetNamedValue(L"name", JsonValue::CreateStringValue(item.name));
        entry.SetNamedValue(L"progress", JsonValue::CreateNumberValue(item.progress));
        result.Append(entry);
    }

    return result;
}

bool isUploading() {
    std::lock_guard lock(mutex);
    return running > 0 || !waiting.empty();
}

void resetUploadSettings() {
    std::lock_guard lock(mutex);
    cachedLimits.reset();
}

UploadOptions uploadOptions() {
    auto upload = jsonObject(settings(), L"upload");
    UploadOptions options;
    options.fileNameType = jsonString(upload, L"fileNameType", L"Random");

    auto expiration = static_cast<int64_t>(jsonNumber(upload, L"expiration"));
    auto maxViews = static_cast<int>(jsonNumber(upload, L"maxViews"));
    auto compression = static_cast<int>(jsonNumber(upload, L"compression"));
    auto password = jsonString(upload, L"password");
    auto folderId = jsonString(upload, L"folderId");

    if (expiration) options.expiration = std::to_wstring(expiration);
    if (maxViews) options.maxViews = maxViews;
    if (compression) options.compression = compression;
    if (!password.empty()) options.password = password;
    if (!folderId.empty()) options.folderId = folderId;
    return options;
}

UploadedFile uploadFile(UploadSource const& source, UploadOptions const& options, UploadProgress const& progress) {
    if (!isConnected()) throw translatedError(t(L"tasks.logInToUpload"));

    auto id = source.id.empty() ? randomHex(8) : source.id;
    uint64_t ticket = 0;

    {
        std::lock_guard lock(mutex);
        ticket = nextTicket++;
        waiting.push_back(ticket);
        progressItems.push_back({id, source.name, 0});
    }

    publish();

    {
        std::unique_lock lock(mutex);
        slots.wait(lock, [&] { return running < MAX_CONCURRENT && waiting.front() == ticket; });
        waiting.pop_front();
        running++;
    }

    slots.notify_all();

    auto finish = [&] {
        bool isIdle = false;

        {
            std::lock_guard lock(mutex);
            running--;
            std::erase_if(progressItems, [&](auto const& item) { return item.id == id; });
            isIdle = running == 0 && waiting.empty();
        }

        slots.notify_all();
        publish();
        if (isIdle) notifyIdle();
    };

    try {
        auto result = runJob(source, id, options, progress);
        finish();
        return result;
    } catch (...) {
        finish();
        throw;
    }
}

std::wstring shortenUrl(std::wstring const& url) {
    try {
        JsonObject body;
        body.SetNamedValue(L"url", JsonValue::CreateStringValue(url));

        auto link = api(L"POST", L"/api/links", body);
        return jsonString(link.GetObject(), L"shortUrl");
    } catch (ApiError const& error) {
        if (error.status == 401) handleUnauthorized();
        throw;
    }
}

}
