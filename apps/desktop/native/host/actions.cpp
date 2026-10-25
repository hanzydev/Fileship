#include "app.h"

#include <host/recording.h>
#include <shared/log.h>

#include <objbase.h>
#include <shobjidl.h>
#include <winrt/base.h>

#include <mutex>
#include <regex>
#include <thread>

namespace fileship::host {

namespace {

constexpr int64_t REPEAT_WINDOW_MS = 400;

std::mutex mutex;
std::map<std::wstring, int64_t> lastRuns;

void uploadPath(std::filesystem::path const& path) {
    handleFile({path, {}, path.filename().wstring(), mimeTypeOf(path), {}});
}

std::optional<std::filesystem::path> clipboardFile() {
    auto path = readClipboardFilePath();
    std::error_code error;
    return path && std::filesystem::is_regular_file(*path, error) ? path : std::nullopt;
}

void uploadClipboard() {
    if (auto path = clipboardFile()) return uploadPath(*path);

    if (auto image = readClipboardImage()) {
        auto base = formatFileName(jsonString(jsonObject(settings(), L"save"), L"pattern", L"%yyyy-%mo-%d_%h-%mi-%s"));
        auto png = core::encodeImage(image->pixels.data(), image->width, image->height, {});
        return handleFile({{}, std::vector<char>(png.begin(), png.end()), (base.empty() ? L"clipboard" : base) + L".png",
                           L"image/png", {}});
    }

    auto text = readClipboardText();
    if (text.find_first_not_of(L" \t\r\n") == std::wstring::npos)
        return notify(t(L"app.nothingToUpload"), t(L"app.nothingToUploadDescription"));

    auto utf8 = toUtf8(text);
    handleFile({{}, std::vector<char>(utf8.begin(), utf8.end()), L"clipboard.txt", L"text/plain", {}});
}

std::vector<std::filesystem::path> pickFiles() {
    std::vector<std::filesystem::path> paths;

    auto dialog = winrt::create_instance<IFileOpenDialog>(CLSID_FileOpenDialog);
    FILEOPENDIALOGOPTIONS options{};
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_ALLOWMULTISELECT | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST);
    auto title = t(L"app.uploadDialogTitle");
    dialog->SetTitle(title.c_str());

    if (FAILED(dialog->Show(nullptr))) return paths;

    winrt::com_ptr<IShellItemArray> items;
    if (FAILED(dialog->GetResults(items.put()))) return paths;

    DWORD count = 0;
    items->GetCount(&count);

    for (DWORD index = 0; index < count; index++) {
        winrt::com_ptr<IShellItem> item;
        PWSTR path = nullptr;

        if (SUCCEEDED(items->GetItemAt(index, item.put())) && SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
            paths.emplace_back(path);
            CoTaskMemFree(path);
        }
    }

    return paths;
}

void uploadFromDialog() {
    auto paths = runOnMainThread<std::vector<std::filesystem::path>>(pickFiles);
    uploadPaths(paths);
}

void shortenClipboard() {
    auto text = readClipboardText();
    auto start = text.find_first_not_of(L" \t\r\n");
    text = start == std::wstring::npos ? L"" : text.substr(start, text.find_last_not_of(L" \t\r\n") - start + 1);

    static std::wregex const link(LR"(^https?://\S+$)", std::regex::icase);
    if (!std::regex_match(text, link)) return notify(t(L"app.noLinkToShorten"), t(L"app.noLinkToShortenDescription"));

    auto id = addHistory({L"link", text, 0, std::nullopt, std::nullopt});

    try {
        auto shortUrl = shortenUrl(text);
        auto afterUpload = jsonObject(settings(), L"afterUpload");

        updateHistory(id, {{L"url", shortUrl}});

        bool copyUrl = jsonBool(afterUpload, L"copyUrl", true);
        if (copyUrl) writeClipboardText(shortUrl);
        playUploaded();
        if (jsonBool(afterUpload, L"openUrl")) openWebUrl(shortUrl);

        if (jsonBool(afterUpload, L"notify", true)) {
            notify(t(copyUrl ? L"app.linkShortened" : L"app.linkShortenedOnly"), shortUrl,
                   [shortUrl] { openWebUrl(shortUrl); });
        }
    } catch (std::exception const& error) {
        updateHistory(id, {{L"error", errorMessage(error)}});
        notify(t(L"app.shortenFailed"), errorMessage(error));
    }
}

void withShot(std::optional<Shot> const& shot) {
    if (!shot) return notify(t(L"app.nothingToCapture"), t(L"app.noActiveWindow"));
    handleImage(shot->image, shot->title);
}

std::map<std::wstring, std::function<void()>> const& handlers() {
    static std::map<std::wstring, std::function<void()>> const actions{
        {L"captureFullscreen", [] { withShot(captureFullscreen()); }},
        {L"captureWindow", [] { withShot(captureActiveWindow()); }},
        {L"captureMonitor", [] { withShot(captureActiveMonitor()); }},
        {L"captureRegion", [] { openRegionCapture(); }},
        {L"captureLastRegion", [] { captureLastRegion(); }},
        {L"captureText", [] { captureText(); }},
        {L"recordVideo", [] { recording::toggleRecording(L"video"); }},
        {L"recordGif", [] { recording::toggleRecording(L"gif"); }},
        {L"uploadClipboard", uploadClipboard},
        {L"uploadFile", uploadFromDialog},
        {L"shortenClipboard", shortenClipboard},
        {L"openMain", [] { showMainWindow(); }},
    };

    return actions;
}

bool needsAccount(std::wstring const& action) {
    return action == L"captureText" || action == L"uploadClipboard" || action == L"uploadFile" ||
           action == L"shortenClipboard";
}

void runGuarded(std::wstring const& name, bool requiresAccount, std::function<void()> const& task) {
    if (requiresAccount && !isConnected()) {
        showMainWindow(L"/connect");
        return;
    }

    try {
        task();
    } catch (std::exception const& error) {
        logError(name + L" failed: " + errorMessage(error));
        notify(t(L"app.somethingWentWrong"), errorMessage(error));
    } catch (winrt::hresult_error const& error) {
        logError(name + L" failed: " + std::wstring(error.message()));
        notify(t(L"app.somethingWentWrong"), std::wstring(error.message()));
    }
}

}

void runAction(std::wstring const& action) {
    auto found = handlers().find(action);
    if (found == handlers().end()) return;

    {
        std::lock_guard lock(mutex);
        auto now = nowMs();
        auto last = lastRuns.find(action);
        bool isRepeat = last != lastRuns.end() && now - last->second < REPEAT_WINDOW_MS;

        lastRuns[action] = now;
        if (isRepeat) return;
    }

    runAsync([action, task = found->second] { runGuarded(L"Action " + action, needsAccount(action), task); });
}

void uploadPaths(std::vector<std::filesystem::path> const& paths) {
    runGuarded(L"Upload", true, [&] {
        std::vector<std::thread> uploads;
        for (auto const& path : paths) uploads.emplace_back([path] { runGuarded(L"Upload", false, [&] { uploadPath(path); }); });
        for (auto& upload : uploads) upload.join();
    });
}

}
