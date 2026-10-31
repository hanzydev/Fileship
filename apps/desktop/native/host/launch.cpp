#include "app.h"

#include <host/harbor.h>

#include <algorithm>
#include <map>
#include <mutex>

namespace fileship::host {

namespace {

constexpr int BATCH_MS = 400;

std::mutex mutex;
std::vector<std::filesystem::path> pendingUploads;
uint64_t uploadGeneration = 0;
std::map<std::wstring, std::vector<std::wstring>> pendingActions;
std::map<std::wstring, uint64_t> actionGenerations;

void queueUploads(std::vector<std::filesystem::path> const& paths) {
    uint64_t generation = 0;

    {
        std::lock_guard lock(mutex);
        pendingUploads.insert(pendingUploads.end(), paths.begin(), paths.end());
        generation = ++uploadGeneration;
    }

    runAfter(BATCH_MS, [generation] {
        std::vector<std::filesystem::path> batch;

        {
            std::lock_guard lock(mutex);
            if (generation != uploadGeneration) return;
            batch.swap(pendingUploads);
        }

        if (!batch.empty()) uploadPaths(batch);
    });
}

void queueHarborAction(std::wstring const& action, std::vector<std::wstring> const& paths) {
    uint64_t generation = 0;

    {
        std::lock_guard lock(mutex);
        auto& pending = pendingActions[action];
        pending.insert(pending.end(), paths.begin(), paths.end());
        generation = ++actionGenerations[action];
    }

    runAfter(BATCH_MS, [action, generation] {
        std::vector<std::wstring> batch;

        {
            std::lock_guard lock(mutex);
            if (actionGenerations[action] != generation) return;
            batch = std::move(pendingActions[action]);
            pendingActions.erase(action);
            actionGenerations.erase(action);
        }

        harbor::runHarborAction(action, batch);
    });
}

std::optional<std::vector<std::wstring>> launchArgs(std::vector<std::wstring> const& arguments, std::wstring const& name) {
    auto found = std::find(arguments.begin(), arguments.end(), name);
    if (found == arguments.end()) return std::nullopt;

    std::vector<std::wstring> result;
    for (auto item = found + 1; item != arguments.end(); item++) {
        if (!item->starts_with(L"-")) result.push_back(*item);
    }

    return result;
}

}

bool handleLaunch(std::vector<std::wstring> const& arguments, std::filesystem::path const& cwd) {
    auto targets = [&](std::wstring const& name) -> std::optional<std::vector<std::wstring>> {
        auto values = launchArgs(arguments, name);
        if (!values) return std::nullopt;

        std::vector<std::wstring> result;
        for (auto const& value : *values) {
            auto path = (cwd / value).lexically_normal();
            if (path == hostExecutable()) continue;
            result.push_back(path.wstring());
        }

        return result;
    };

    if (auto action = launchArgs(arguments, L"--harbor-action"); action && !action->empty()) {
        auto paths = targets(L"--harbor-action").value();
        if (!paths.empty()) paths.erase(paths.begin());
        queueHarborAction(action->front(), paths);
        return true;
    }

    if (auto upload = targets(L"--upload")) {
        std::vector<std::filesystem::path> paths(upload->begin(), upload->end());
        queueUploads(paths);
        return true;
    }

    if (auto action = argumentOf(arguments, L"--action"); !action.empty()) {
        runAction(action);
        return true;
    }

    auto route = argumentOf(arguments, L"--show");
    if (!route.empty()) {
        showMainWindow(route);
        return true;
    }

    return false;
}

}
