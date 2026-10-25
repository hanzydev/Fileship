#include "app.h"
#include "rpc.h"

#include <shared/log.h>

#include <mutex>

namespace fileship::host {

namespace {

constexpr ULONGLONG LAUNCH_TIMEOUT_MS = 30000;

std::mutex mutex;
ULONGLONG launchedAt = 0;
std::vector<std::pair<std::wstring, JsonObject>> pendingEvents;

std::wstring quote(std::wstring const& argument) {
    if (!argument.empty() && argument.find_first_of(L" \t\"") == std::wstring::npos) return argument;

    std::wstring result = L"\"";
    size_t backslashes = 0;

    for (auto character : argument) {
        if (character == L'\\') {
            backslashes++;
            continue;
        }

        result.append(character == L'"' ? backslashes * 2 + 1 : backslashes, L'\\');
        backslashes = 0;
        result += character;
    }

    result.append(backslashes * 2, L'\\');
    return result + L"\"";
}

std::wstring environmentWith(std::wstring const& name, std::wstring const& value) {
    std::wstring block;
    auto strings = GetEnvironmentStringsW();

    for (auto entry = strings; *entry; entry += wcslen(entry) + 1) {
        std::wstring item(entry);
        if (_wcsnicmp(item.c_str(), (name + L"=").c_str(), name.size() + 1) == 0) continue;
        block += item;
        block.push_back(L'\0');
    }

    FreeEnvironmentStringsW(strings);
    block += name + L"=" + value;
    block.push_back(L'\0');
    block.push_back(L'\0');
    return block;
}

JsonObject routePayload(std::wstring const& route) {
    JsonObject payload;
    payload.SetNamedValue(L"route", JsonValue::CreateStringValue(route));
    return payload;
}

void requestWindow(std::wstring const& event, std::wstring const& route, std::vector<std::wstring> arguments) {
    if (hasWindowClient()) {
        broadcast(event, routePayload(route));
        return;
    }

    {
        std::lock_guard lock(mutex);
        if (launchedAt && GetTickCount64() - launchedAt < LAUNCH_TIMEOUT_MS) {
            pendingEvents.emplace_back(event, routePayload(route));
            return;
        }

        launchedAt = GetTickCount64();
        pendingEvents.clear();
    }

    launchElectron(arguments);
}

}

void launchElectron(std::vector<std::wstring> const& arguments) {
    auto const& options = launchOptions();
    std::wstring command;
    std::filesystem::path directory;

    if (isPackaged()) {
        auto executable = resourcesPath().parent_path().parent_path() / L"Fileship.exe";
        command = quote(executable.wstring());
        directory = executable.parent_path();
    } else {
        if (options.electronCommand.empty()) {
            logWarn(L"No Electron command was given, cannot open a window");
            return;
        }

        command = options.electronCommand;
        directory = std::filesystem::current_path();
    }

    command += L" --host-pipe " + quote(pipeName());
    if (!options.userData.empty()) command += L" --user-data-dir=" + quote(userDataPath().wstring());
    for (auto const& argument : arguments) command += L" " + quote(argument);

    auto environment = options.rendererUrl.empty() ? std::wstring()
                                                   : environmentWith(L"DESKTOP_RENDERER_URL", options.rendererUrl);

    STARTUPINFOW startup{sizeof(startup)};
    PROCESS_INFORMATION process{};

    logInfo(L"Starting Electron: " + command);

    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT,
                        environment.empty() ? nullptr : environment.data(), directory.c_str(), &startup, &process)) {
        logError(L"Could not start Electron: " + std::to_wstring(GetLastError()));
        std::lock_guard lock(mutex);
        launchedAt = 0;
        return;
    }

    AllowSetForegroundWindow(process.dwProcessId);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
}

void flushWindowEvents(uint64_t client) {
    std::vector<std::pair<std::wstring, JsonObject>> events;

    {
        std::lock_guard lock(mutex);
        launchedAt = 0;
        events.swap(pendingEvents);
    }

    for (auto const& [event, payload] : events) sendTo(client, event, payload);
}

void showMainWindow(std::wstring const& route) {
    requestWindow(L"window:show", route, {L"--route", route});
}

void openDashboard(std::wstring const& route) {
    if (instanceUrl().empty() || token().empty()) return;
    requestWindow(L"window:dashboard", route, {L"--dashboard", route});
}

}
