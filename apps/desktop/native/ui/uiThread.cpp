#include "uiThread.h"

#include <windows.h>

#include <winrt/base.h>

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace fileship::ui {

namespace {

constexpr UINT WM_RUN_TASKS = WM_APP + 1;

std::mutex mutex;
std::condition_variable ready;
std::deque<std::function<void()>> tasks;
HWND dispatcher = nullptr;
DWORD threadId = 0;
bool isStarted = false;

void runTasks() {
    while (true) {
        std::function<void()> task;
        {
            std::lock_guard lock(mutex);
            if (tasks.empty()) return;
            task = std::move(tasks.front());
            tasks.pop_front();
        }
        task();
    }
}

LRESULT CALLBACK handleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_RUN_TASKS) {
        runTasks();
        return 0;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

void run() {
    winrt::init_apartment(winrt::apartment_type::single_threaded);
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = handleMessage;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = L"FileshipUiDispatcher";
    RegisterClassW(&windowClass);

    HWND window = CreateWindowExW(0, windowClass.lpszClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr,
                                  windowClass.hInstance, nullptr);
    {
        std::lock_guard lock(mutex);
        dispatcher = window;
        threadId = GetCurrentThreadId();
    }
    ready.notify_all();
    runTasks();

    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}

}

void runOnUi(std::function<void()> task) {
    std::unique_lock lock(mutex);
    tasks.push_back(std::move(task));

    if (!isStarted) {
        isStarted = true;
        std::thread(run).detach();
    }

    ready.wait(lock, [] { return dispatcher != nullptr; });
    PostMessageW(dispatcher, WM_RUN_TASKS, 0, 0);
}

bool isUiThread() {
    std::lock_guard lock(mutex);
    return threadId == GetCurrentThreadId();
}

}
