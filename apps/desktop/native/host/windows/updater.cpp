#include <host/app.h>
#include <shared/log.h>

#include <windows.h>

namespace fileship::host {

std::wstring updateManifestName() {
    return L"latest.yml";
}

bool canUpdateItself() {
    return true;
}

bool isUpdateFile(std::wstring const& url) {
    return url.ends_with(L".exe");
}

void startInstaller(std::filesystem::path const& path, bool forceRun) {
    std::wstring command = L"\"" + path.wstring() + L"\" --updated /S" + (forceRun ? L" --force-run" : L"");
    STARTUPINFOW startup{sizeof(startup)};
    PROCESS_INFORMATION process{};

    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
                        nullptr, path.parent_path().c_str(), &startup, &process)) {
        logError(L"Could not start the update installer: " + std::to_wstring(GetLastError()));
        return;
    }

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
}

}
