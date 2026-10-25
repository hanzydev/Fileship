#include "app.h"

#include <host/recording.h>
#include <shared/log.h>

#include <shlobj.h>

#include <atomic>
#include <fstream>
#include <mutex>
#include <random>

namespace fileship::host {

namespace {

std::atomic<int> activeTasks = 0;
std::mutex soundsMutex;
std::map<std::wstring, std::vector<char>> sounds;

std::wstring pad(int value, size_t width = 2) {
    auto text = std::to_wstring(value);
    return std::wstring(width > text.size() ? width - text.size() : 0, L'0') + text;
}

void replaceAll(std::wstring& text, std::wstring const& from, std::wstring const& to) {
    for (size_t position = 0; (position = text.find(from, position)) != std::wstring::npos; position += to.size())
        text.replace(position, from.size(), to);
}

std::wstring randomBase36() {
    static thread_local std::mt19937_64 generator{std::random_device{}()};
    static wchar_t const* const DIGITS = L"0123456789abcdefghijklmnopqrstuvwxyz";

    std::wstring result;
    for (int index = 0; index < 10; index++) result += DIGITS[generator() % 36];
    return result;
}

std::filesystem::path soundPath(std::wstring const& name) {
    auto packaged = resourcesPath() / L"sounds" / name;
    if (std::filesystem::exists(packaged)) return packaged;
    return resourcesPath().parent_path().parent_path() / L"resources" / name;
}

bool writeBytes(std::filesystem::path const& path, void const* data, size_t size) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(static_cast<char const*>(data), static_cast<std::streamsize>(size));
    return static_cast<bool>(file);
}

void notifyUploadFailure(std::exception const& error) {
    if (!isQuotaExceeded(error)) {
        notify(t(L"tasks.uploadFailed"), errorMessage(error));
        return;
    }

    notify(t(L"tasks.storageFull"), errorMessage(error), [] { openDashboard(L"/dashboard/settings/storage"); });
}

std::optional<std::wstring> uploadAndFinish(std::wstring const& historyId, UploadSource source,
                                            std::function<void(std::wstring const&)> const& onFailure = {}) {
    try {
        source.id = historyId;
        auto uploaded = uploadFile(source, uploadOptions());
        return finishUpload(historyId, uploaded);
    } catch (std::exception const& error) {
        logWarn(L"Upload failed: " + errorMessage(error));
        updateHistory(historyId, {{L"error", errorMessage(error)}});

        if (onFailure) onFailure(errorMessage(error));
        else notifyUploadFailure(error);

        return std::nullopt;
    }
}

void notifySaved(std::filesystem::path const& path) {
    notify(t(L"tasks.imageSaved"), path.wstring(), [path] { showItemInFolder(path); });
}

void keepImage(std::wstring const& historyId, std::vector<uint8_t> const& png, std::wstring const& name,
               std::wstring const& message) {
    try {
        auto folder = saveFolder();
        std::filesystem::create_directories(folder);

        auto kept = uniquePath(folder, name);
        if (!writeBytes(kept, png.data(), png.size())) throw std::runtime_error("write failed");

        updateHistory(historyId, {{L"localPath", kept.wstring()}});
        notify(t(L"tasks.imageKept"), kept.wstring(), [kept] { showItemInFolder(kept); });
    } catch (std::exception const& error) {
        logWarn(L"Could not keep the screenshot: " + errorMessage(error));
        notify(t(L"tasks.uploadFailed"), message);
    }
}

void moveFile(std::filesystem::path const& from, std::filesystem::path const& to) {
    if (MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_COPY_ALLOWED)) return;
    throw std::runtime_error("move failed");
}

void keepRecording(std::wstring const& historyId, std::filesystem::path const& path, std::wstring const& name,
                   std::wstring const& message) {
    try {
        auto folder = saveFolder();
        std::filesystem::create_directories(folder);

        auto kept = uniquePath(folder, name);
        moveFile(path, kept);

        updateHistory(historyId, {{L"localPath", kept.wstring()}});
        notify(t(L"tasks.recordingKept"), kept.wstring(), [kept] { showItemInFolder(kept); });
    } catch (std::exception const& error) {
        logWarn(L"Could not keep the recording: " + errorMessage(error));
        notify(t(L"tasks.uploadFailed"), message);
    }
}

std::optional<std::wstring> fileThumbnail(UploadSource const& file) {
    if (file.mimeType.rfind(L"image/", 0) != 0) return std::nullopt;

    auto image = file.data.empty()
                     ? core::decodeImageFile(file.path)
                     : core::decodeImage(reinterpret_cast<uint8_t const*>(file.data.data()), file.data.size());
    if (!image) return std::nullopt;

    try {
        return thumbnailOf(*image);
    } catch (std::exception const&) {
        return std::nullopt;
    }
}

}

BusyScope::BusyScope() {
    activeTasks++;
}

BusyScope::~BusyScope() {
    activeTasks--;
    notifyIdle();
}

bool isWorking() {
    return activeTasks > 0;
}

bool isBusy() {
    return isUploading() || recording::isRecording() || isWorking() || isCapturing();
}

std::wstring formatFileName(std::wstring const& pattern, std::wstring const& title) {
    SYSTEMTIME now{};
    GetLocalTime(&now);

    auto result = pattern;
    replaceAll(result, L"%yyyy", std::to_wstring(now.wYear));
    replaceAll(result, L"%mo", pad(now.wMonth));
    replaceAll(result, L"%d", pad(now.wDay));
    replaceAll(result, L"%h", pad(now.wHour));
    replaceAll(result, L"%mi", pad(now.wMinute));
    replaceAll(result, L"%ms", pad(now.wMilliseconds, 3));
    replaceAll(result, L"%s", pad(now.wSecond));
    replaceAll(result, L"%pn", title);
    replaceAll(result, L"%ra", randomBase36());

    std::erase_if(result, [](wchar_t character) { return std::wstring_view(L"\\/:*?\"<>|").find(character) != std::wstring_view::npos; });

    auto start = result.find_first_not_of(L" \t\r\n");
    if (start == std::wstring::npos) return {};
    return result.substr(start, result.find_last_not_of(L" \t\r\n") - start + 1);
}

std::filesystem::path saveFolder() {
    auto folder = jsonString(jsonObject(settings(), L"save"), L"folder");
    if (!folder.empty()) return folder;

    PWSTR pictures = nullptr;
    SHGetKnownFolderPath(FOLDERID_Pictures, 0, nullptr, &pictures);
    std::filesystem::path result = std::filesystem::path(pictures) / L"Fileship";
    CoTaskMemFree(pictures);
    return result;
}

std::filesystem::path uniquePath(std::filesystem::path const& folder, std::wstring const& name) {
    std::filesystem::path file(name);
    auto extension = file.extension().wstring();
    auto base = name.substr(0, name.size() - extension.size());
    auto path = folder / name;

    for (int index = 2; std::filesystem::exists(path); index++)
        path = folder / (base + L" (" + std::to_wstring(index) + L")" + extension);

    return path;
}

void playSoundFile(std::wstring const& name) {
    std::lock_guard lock(soundsMutex);
    auto found = sounds.find(name);

    if (found == sounds.end()) {
        auto bytes = readFileBytes(soundPath(name));
        if (bytes.empty()) {
            logWarn(L"Could not play " + name);
            return;
        }

        found = sounds.emplace(name, std::move(bytes)).first;
    }

    core::playSound(reinterpret_cast<uint8_t const*>(found->second.data()), found->second.size());
}

void playShutter() {
    if (jsonBool(jsonObject(settings(), L"capture"), L"playSound", true)) playSoundFile(L"shutter.wav");
}

void playUploaded() {
    if (jsonBool(jsonObject(settings(), L"afterUpload"), L"playSound", true)) playSoundFile(L"uploaded.wav");
}

std::wstring thumbnailOf(core::Bitmap const& bitmap) {
    core::EncodeOptions options;
    options.format = core::ImageFormat::Jpeg;
    options.quality = 0.75f;
    options.maxWidth = 480;

    auto jpeg = core::encodeImage(bitmap.pixels.data(), bitmap.width, bitmap.height, options);
    return L"data:image/jpeg;base64," + base64(jpeg.data(), jpeg.size());
}

std::wstring finishUpload(std::wstring const& historyId, UploadedFile const& file) {
    auto afterUpload = jsonObject(settings(), L"afterUpload");
    auto url = file.url;
    std::optional<std::wstring> shortUrl;

    if (jsonBool(afterUpload, L"shortenUrl")) {
        try {
            shortUrl = shortenUrl(url);
            url = *shortUrl;
        } catch (std::exception const& error) {
            logWarn(L"Could not shorten the link: " + errorMessage(error));
        }
    }

    updateHistory(historyId, {{L"url", file.url}, {L"shortUrl", shortUrl}, {L"fileId", file.id}, {L"error", std::nullopt}});

    bool copyUrl = jsonBool(afterUpload, L"copyUrl", true);
    if (copyUrl) writeClipboardText(url);
    playUploaded();
    if (jsonBool(afterUpload, L"openUrl")) openWebUrl(url);

    if (jsonBool(afterUpload, L"notify", true)) {
        notify(t(copyUrl ? L"tasks.uploadedLinkCopied" : L"tasks.uploaded"), url, [url] { openWebUrl(url); });
    }

    return url;
}

void handleImage(core::Bitmap const& image, std::wstring const& title, std::wstring const& kind) {
    BusyScope busy;

    auto current = settings();
    auto afterCapture = jsonObject(current, L"afterCapture");
    auto base = formatFileName(jsonString(jsonObject(current, L"save"), L"pattern", L"%yyyy-%mo-%d_%h-%mi-%s"), title);
    auto name = (base.empty() ? L"capture" : base) + L".png";

    auto png = core::encodeImage(image.pixels.data(), image.width, image.height, {});
    auto thumbnail = thumbnailOf(image);

    std::optional<std::wstring> localPath;

    if (jsonBool(afterCapture, L"copyImage")) writeClipboardImage(png, image);

    if (jsonBool(afterCapture, L"saveToFile")) {
        auto folder = saveFolder();
        std::filesystem::create_directories(folder);
        auto path = uniquePath(folder, name);
        writeBytes(path, png.data(), png.size());
        localPath = path.wstring();
    }

    auto id = addHistory({kind, name, static_cast<int64_t>(png.size()), localPath, thumbnail});

    if (jsonBool(afterCapture, L"upload", true)) {
        UploadSource source{{}, std::vector<char>(png.begin(), png.end()), name, L"image/png", {}};
        std::function<void(std::wstring const&)> onFailure;
        if (!localPath) onFailure = [&](std::wstring const& message) { keepImage(id, png, name, message); };

        uploadAndFinish(id, std::move(source), onFailure);
    } else if (localPath) {
        notifySaved(*localPath);
    } else {
        if (!jsonBool(afterCapture, L"copyImage")) writeClipboardImage(png, image);
        notify(t(L"tasks.screenshotCopied"), t(L"tasks.uploadsOff"));
    }
}

void handleFile(UploadSource const& source) {
    BusyScope busy;

    std::error_code error;
    auto size = source.data.empty() ? static_cast<int64_t>(std::filesystem::file_size(source.path, error))
                                    : static_cast<int64_t>(source.data.size());

    std::optional<std::wstring> localPath;
    if (source.data.empty() && !source.path.empty()) localPath = source.path.wstring();

    auto id = addHistory({L"file", source.name, size, localPath, fileThumbnail(source)});
    uploadAndFinish(id, source);
}

void handleRecording(RecordingOutput const& output) {
    BusyScope busy;

    auto name = output.name.empty() ? output.path.filename().wstring() : output.name;
    std::error_code error;
    auto size = static_cast<int64_t>(std::filesystem::file_size(output.path, error));

    std::optional<std::wstring> thumbnail;
    if (!output.thumbnailJpeg.empty())
        thumbnail = L"data:image/jpeg;base64," + base64(output.thumbnailJpeg.data(), output.thumbnailJpeg.size());

    std::optional<std::wstring> localPath;
    if (output.isSaved) localPath = output.path.wstring();

    auto id = addHistory({output.kind, name, size, localPath, thumbnail});

    if (jsonBool(jsonObject(settings(), L"afterCapture"), L"upload", true)) {
        UploadSource source{output.path, {}, name, mimeTypeOf(output.path), {}};
        std::function<void(std::wstring const&)> onFailure;
        if (!output.isSaved) {
            onFailure = [&](std::wstring const& message) { keepRecording(id, output.path, name, message); };
        }

        uploadAndFinish(id, std::move(source), onFailure);
    } else {
        auto path = output.path;
        notify(t(L"tasks.recordingSaved"), path.wstring(), [path] { showItemInFolder(path); });
    }
}

}
