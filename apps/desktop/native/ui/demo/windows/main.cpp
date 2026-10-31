#include <ui/demo/fixtures.h>

#include <ui/fixture.h>
#include <ui/overlay.h>
#include <ui/recorderUi.h>

#include <shared/i18n.h>
#include <shared/log.h>
#include <shared/utf8.h>

#include <windows.h>

#include <dwmapi.h>
#include <psapi.h>
#include <shellapi.h>
#include <shellscalingapi.h>
#include <wincodec.h>

#include <winrt/base.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "shcore.lib")

using namespace fileship;
using namespace fileship::ui;
using namespace fileship::ui::demo;

namespace fileship::ui::demo {

void print(std::string const& text) {
    static std::mutex mutex;
    std::lock_guard lock(mutex);
    fputs(text.c_str(), stdout);
    fputs("\n", stdout);
    fflush(stdout);
}

void savePng(std::filesystem::path const& path, int width, int height, std::vector<uint8_t> const& bgra) {
    winrt::com_ptr<IWICImagingFactory> factory;
    winrt::check_hresult(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                          IID_PPV_ARGS(factory.put())));

    winrt::com_ptr<IWICStream> stream;
    winrt::check_hresult(factory->CreateStream(stream.put()));
    winrt::check_hresult(stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE));

    winrt::com_ptr<IWICBitmapEncoder> encoder;
    winrt::check_hresult(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, encoder.put()));
    winrt::check_hresult(encoder->Initialize(stream.get(), WICBitmapEncoderNoCache));

    winrt::com_ptr<IWICBitmapFrameEncode> frame;
    winrt::check_hresult(encoder->CreateNewFrame(frame.put(), nullptr));
    winrt::check_hresult(frame->Initialize(nullptr));
    winrt::check_hresult(frame->SetSize(width, height));
    WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
    winrt::check_hresult(frame->SetPixelFormat(&format));
    winrt::check_hresult(frame->WritePixels(height, width * 4, static_cast<UINT>(bgra.size()),
                                            const_cast<BYTE*>(bgra.data())));
    winrt::check_hresult(frame->Commit());
    winrt::check_hresult(encoder->Commit());
}

}

namespace {

std::vector<uint8_t> captureRect(RECT const& rect) {
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC screen = GetDC(nullptr);
    HDC memory = CreateCompatibleDC(screen);
    HBITMAP bitmap = CreateDIBSection(screen, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    auto previous = SelectObject(memory, bitmap);
    BitBlt(memory, 0, 0, width, height, screen, rect.left, rect.top, SRCCOPY | CAPTUREBLT);

    std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4);
    memcpy(pixels.data(), bits, pixels.size());
    for (size_t index = 3; index < pixels.size(); index += 4) pixels[index] = 255;

    SelectObject(memory, previous);
    DeleteObject(bitmap);
    DeleteDC(memory);
    ReleaseDC(nullptr, screen);
    return pixels;
}

BOOL CALLBACK collectChild(HWND child, LPARAM parameter) {
    auto& controls = *reinterpret_cast<std::vector<ScreenRect>*>(parameter);
    RECT rect{};
    if (IsWindowVisible(child) && GetWindowRect(child, &rect)) {
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        if (width > 4 && height > 4 && controls.size() < 3000)
            controls.push_back({rect.left, rect.top, width, height});
    }
    return TRUE;
}

BOOL CALLBACK collectWindow(HWND window, LPARAM parameter) {
    auto& windows = *reinterpret_cast<std::vector<WindowTarget>*>(parameter);
    if (!IsWindowVisible(window) || IsIconic(window)) return TRUE;

    DWORD cloaked = 0;
    if (SUCCEEDED(DwmGetWindowAttribute(window, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && cloaked) return TRUE;

    auto exStyle = GetWindowLongPtrW(window, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW || exStyle & WS_EX_NOACTIVATE) return TRUE;

    RECT rect{};
    if (FAILED(DwmGetWindowAttribute(window, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(rect))) &&
        !GetWindowRect(window, &rect))
        return TRUE;
    if (rect.right <= rect.left || rect.bottom <= rect.top) return TRUE;

    WindowTarget target;
    target.rect = {rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top};
    wchar_t title[256]{};
    GetWindowTextW(window, title, 256);
    target.title = title;
    EnumChildWindows(window, collectChild, reinterpret_cast<LPARAM>(&target.controls));
    windows.push_back(std::move(target));
    return TRUE;
}

OverlayRequest captureRequest(OverlayPurpose purpose, ThemeMode theme) {
    OverlayRequest request;
    request.purpose = purpose;
    request.theme = theme;

    std::vector<std::pair<HMONITOR, MONITORINFOEXW>> monitors;
    EnumDisplayMonitors(
        nullptr, nullptr,
        [](HMONITOR monitor, HDC, LPRECT, LPARAM data) -> BOOL {
            MONITORINFOEXW info{};
            info.cbSize = sizeof(info);
            GetMonitorInfoW(monitor, &info);
            reinterpret_cast<std::vector<std::pair<HMONITOR, MONITORINFOEXW>>*>(data)->push_back({monitor, info});
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&monitors));

    std::sort(monitors.begin(), monitors.end(), [](auto const& a, auto const& b) {
        bool aPrimary = a.second.dwFlags & MONITORINFOF_PRIMARY;
        bool bPrimary = b.second.dwFlags & MONITORINFOF_PRIMARY;
        if (aPrimary != bPrimary) return aPrimary;
        return a.second.rcMonitor.left < b.second.rcMonitor.left;
    });

    RECT bounds{INT_MAX, INT_MAX, INT_MIN, INT_MIN};
    int64_t id = 1;
    for (auto const& [monitor, info] : monitors) {
        auto const& rect = info.rcMonitor;
        UINT dpiX = 96;
        UINT dpiY = 96;
        GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);

        DisplaySnapshot display;
        display.id = id++;
        display.rect = {rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top};
        display.scaleFactor = dpiX / 96.0;
        display.isPrimary = info.dwFlags & MONITORINFOF_PRIMARY;
        display.bgra = captureRect(rect);
        request.displays.push_back(std::move(display));

        bounds.left = std::min(bounds.left, rect.left);
        bounds.top = std::min(bounds.top, rect.top);
        bounds.right = std::max(bounds.right, rect.right);
        bounds.bottom = std::max(bounds.bottom, rect.bottom);
    }

    request.bounds = {bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top};
    EnumWindows(collectWindow, reinterpret_cast<LPARAM>(&request.windows));
    POINT cursor{};
    GetCursorPos(&cursor);
    request.cursor = {cursor.x, cursor.y};
    return request;
}

struct Memory {
    size_t workingSet = 0;
    size_t privateBytes = 0;
};

Memory memory() {
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb = sizeof(counters);
    GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters), sizeof(counters));
    return {counters.WorkingSetSize, counters.PrivateUsage};
}

std::string megabytes(size_t bytes) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.1f MB", bytes / 1048576.0);
    return buffer;
}

struct Waiter {
    std::mutex mutex;
    std::condition_variable condition;
    bool isDone = false;
    OverlayResult result;

    void set(OverlayResult value) {
        std::lock_guard lock(mutex);
        result = std::move(value);
        isDone = true;
        condition.notify_all();
    }

    void wait() {
        std::unique_lock lock(mutex);
        condition.wait(lock, [&] { return isDone; });
    }
};

HWND findOverlay() {
    for (int attempt = 0; attempt < 200; attempt++) {
        if (HWND window = FindWindowW(L"FileshipOverlay", nullptr); window && IsWindowVisible(window)) return window;
        Sleep(10);
    }
    return nullptr;
}

void sendCtrlKey(WORD key, bool shift) {
    std::vector<INPUT> inputs;
    auto push = [&](WORD vk, bool up) {
        INPUT input{};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vk;
        input.ki.dwFlags = up ? KEYEVENTF_KEYUP : 0;
        inputs.push_back(input);
    };
    push(VK_CONTROL, false);
    if (shift) push(VK_SHIFT, false);
    push(key, false);
    push(key, true);
    if (shift) push(VK_SHIFT, true);
    push(VK_CONTROL, true);
    SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
}

void runScript(std::filesystem::path const& path, HWND window, ScreenRect const& bounds) {
    std::ifstream file(path);
    std::string line;

    while (std::getline(file, line) && IsWindow(window)) {
        std::istringstream stream(line);
        std::string command;
        stream >> command;
        if (command.empty() || command[0] == '#') continue;

        int x = 0;
        int y = 0;
        auto point = [&]() { return MAKELPARAM(x, y); };

        if (command == "move" || command == "smove") {
            stream >> x >> y;
            PostMessageW(window, WM_MOUSEMOVE, command == "smove" ? MK_SHIFT : 0, point());
        } else if (command == "drag" || command == "sdrag") {
            int toX = 0;
            int toY = 0;
            int steps = 10;
            stream >> x >> y >> toX >> toY >> steps;
            WPARAM shift = command == "sdrag" ? MK_SHIFT : 0;
            PostMessageW(window, WM_MOUSEMOVE, shift, point());
            PostMessageW(window, WM_LBUTTONDOWN, MK_LBUTTON | shift, point());
            Sleep(20);
            for (int step = 1; step <= steps; step++) {
                int px = x + (toX - x) * step / steps;
                int py = y + (toY - y) * step / steps;
                PostMessageW(window, WM_MOUSEMOVE, MK_LBUTTON | shift, MAKELPARAM(px, py));
                Sleep(16);
            }
            PostMessageW(window, WM_LBUTTONUP, shift, MAKELPARAM(toX, toY));
        } else if (command == "click") {
            stream >> x >> y;
            PostMessageW(window, WM_MOUSEMOVE, 0, point());
            PostMessageW(window, WM_LBUTTONDOWN, MK_LBUTTON, point());
            PostMessageW(window, WM_LBUTTONUP, 0, point());
        } else if (command == "rclick") {
            stream >> x >> y;
            PostMessageW(window, WM_RBUTTONDOWN, MK_RBUTTON, point());
            PostMessageW(window, WM_RBUTTONUP, 0, point());
        } else if (command == "key") {
            std::string name;
            stream >> name;
            UINT key = name == "esc"     ? VK_ESCAPE
                       : name == "enter" ? VK_RETURN
                       : name == "space" ? VK_SPACE
                       : name == "del"   ? VK_DELETE
                       : name == "back"  ? VK_BACK
                       : name == "left"  ? VK_LEFT
                       : name == "right" ? VK_RIGHT
                                         : static_cast<UINT>(std::toupper(static_cast<unsigned char>(name[0])));
            PostMessageW(window, WM_KEYDOWN, key, 0);
            PostMessageW(window, WM_KEYUP, key, 0);
        } else if (command == "char") {
            std::string text;
            std::getline(stream, text);
            if (!text.empty() && text[0] == ' ') text.erase(0, 1);
            for (wchar_t character : toWide(text)) PostMessageW(window, WM_CHAR, character, 0);
        } else if (command == "ctrl" || command == "ctrlshift") {
            std::string key;
            stream >> key;
            SetForegroundWindow(window);
            sendCtrlKey(static_cast<WORD>(std::toupper(static_cast<unsigned char>(key[0]))), command == "ctrlshift");
        } else if (command == "wait") {
            int milliseconds = 0;
            stream >> milliseconds;
            Sleep(milliseconds);
        } else if (command == "shot") {
            std::string name;
            stream >> name;
            Sleep(200);
            RECT rect{bounds.x, bounds.y, bounds.x + bounds.width, bounds.y + bounds.height};
            savePng(name, bounds.width, bounds.height, captureRect(rect));
            print("shot " + name);
        } else if (command == "memory") {
            auto value = memory();
            print("memory working set " + megabytes(value.workingSet) + ", private " + megabytes(value.privateBytes));
        }

        Sleep(30);
    }
}

OverlayRequest syntheticRequest(OverlayPurpose purpose, ThemeMode theme) {
    HMONITOR monitor = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO info{sizeof(info)};
    GetMonitorInfoW(monitor, &info);
    auto const& rect = info.rcMonitor;
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    UINT dpiX = 96;
    UINT dpiY = 96;
    GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);

    OverlayRequest request;
    request.purpose = purpose;
    request.theme = theme;
    request.bounds = {rect.left, rect.top, width, height};
    request.cursor = {rect.left + 300, rect.top + 330};

    DisplaySnapshot display;
    display.id = 1;
    display.rect = request.bounds;
    display.scaleFactor = dpiX / 96.0;
    display.isPrimary = true;
    display.bgra = fixtureSnapshot(width, height);
    request.displays.push_back(std::move(display));

    WindowTarget first;
    first.rect = {rect.left + 120, rect.top + 100, 640, 460};
    first.controls = {{rect.left + 160, rect.top + 170, 260, 40}};
    WindowTarget second;
    second.rect = {rect.left + 860, rect.top + 180, 620, 580};
    request.windows = {first, second};
    return request;
}

OverlayPurpose parsePurpose(std::string const& value) {
    if (value == "ocr") return OverlayPurpose::Ocr;
    if (value == "video") return OverlayPurpose::Video;
    if (value == "gif") return OverlayPurpose::Gif;
    return OverlayPurpose::Capture;
}

int runOverlay(OverlayPurpose purpose, ThemeMode theme, std::filesystem::path const& output,
               std::filesystem::path const& script, bool synthetic) {
    auto before = memory();
    print("before capture: working set " + megabytes(before.workingSet) + ", private " + megabytes(before.privateBytes));

    auto request = synthetic ? syntheticRequest(purpose, theme) : captureRequest(purpose, theme);
    auto bounds = request.bounds;
    auto captured = memory();
    print("after GDI capture: working set " + megabytes(captured.workingSet) + ", private " +
          megabytes(captured.privateBytes));

    Waiter waiter;
    std::atomic<size_t> peakWorkingSet{0};
    std::atomic<size_t> peakPrivate{0};
    std::atomic<bool> isSampling{true};

    std::thread sampler([&] {
        while (isSampling) {
            auto value = memory();
            peakWorkingSet = std::max(peakWorkingSet.load(), value.workingSet);
            peakPrivate = std::max(peakPrivate.load(), value.privateBytes);
            Sleep(20);
        }
    });

    double start = static_cast<double>(GetTickCount64());
    if (!openOverlay(std::move(request), [&](OverlayResult result) { waiter.set(std::move(result)); })) {
        print("openOverlay refused");
        return 1;
    }

    HWND window = findOverlay();
    print("overlay visible after " + std::to_string(static_cast<int>(GetTickCount64() - start)) + " ms");
    Sleep(300);
    auto open = memory();
    print("overlay open: working set " + megabytes(open.workingSet) + ", private " + megabytes(open.privateBytes));

    if (!script.empty() && window) runScript(script, window, bounds);

    waiter.wait();
    isSampling = false;
    sampler.join();
    Sleep(200);

    auto after = memory();
    print("peak while open: working set " + megabytes(peakWorkingSet) + ", private " + megabytes(peakPrivate));
    print("after close: working set " + megabytes(after.workingSet) + ", private " + megabytes(after.privateBytes));

    auto const& result = waiter.result;
    switch (result.type) {
    case OverlayResultType::Cancel:
        print("result: cancel");
        break;
    case OverlayResultType::Area:
        print("result: area " + std::to_string(result.rect.x) + "," + std::to_string(result.rect.y) + " " +
              std::to_string(result.rect.width) + "x" + std::to_string(result.rect.height));
        break;
    case OverlayResultType::Region:
        print("result: region " + std::to_string(result.rect.x) + "," + std::to_string(result.rect.y) + " " +
              std::to_string(result.width) + "x" + std::to_string(result.height));
        savePng(output, result.width, result.height, result.bgra);
        print("saved " + toUtf8(output.wstring()));
        break;
    }

    return 0;
}

int runRecorder(ThemeMode theme, int seconds, bool flag) {
    POINT cursor{};
    GetCursorPos(&cursor);
    HMONITOR monitor = MonitorFromPoint(cursor, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO info{sizeof(info)};
    GetMonitorInfoW(monitor, &info);
    auto const& rect = info.rcMonitor;
    int width = (rect.right - rect.left) / 2;
    int height = (rect.bottom - rect.top) / 2;
    ScreenRect area{rect.left + width / 2, rect.top + height / 3, width, height};

    RecorderViewState state;
    state.sources = {AudioSource::Microphone, AudioSource::System};
    std::mutex mutex;

    RecorderUiCallbacks callbacks;
    callbacks.onVisible = [] { print("recorder visible"); };
    callbacks.onControl = [&](RecorderAction action) {
        static char const* names[] = {"stop", "pause", "resume", "abort"};
        print(std::string("control ") + names[static_cast<int>(action)]);
        std::lock_guard lock(mutex);
        if (action == RecorderAction::Pause) state.phase = RecorderPhase::Paused;
        if (action == RecorderAction::Resume) state.phase = RecorderPhase::Recording;
        updateRecorderUi(state);
    };
    callbacks.onMute = [&](AudioSource source, bool muted) {
        print(std::string("mute ") + (source == AudioSource::Microphone ? "microphone " : "system ") +
              (muted ? "on" : "off"));
        std::lock_guard lock(mutex);
        auto& list = state.muted;
        list.erase(std::remove(list.begin(), list.end(), source), list.end());
        if (muted) list.push_back(source);
        updateRecorderUi(state);
    };

    showRecorderUi(area, theme, callbacks);
    updateRecorderUi(state);
    Sleep(1000);

    {
        std::lock_guard lock(mutex);
        state.phase = RecorderPhase::Countdown;
        state.countdown = 3;
        updateRecorderUi(state);
    }
    Sleep(1000);

    auto epoch = [] {
        FILETIME time{};
        GetSystemTimePreciseAsFileTime(&time);
        ULARGE_INTEGER value{{time.dwLowDateTime, time.dwHighDateTime}};
        return static_cast<int64_t>((value.QuadPart - 116444736000000000ULL) / 10000);
    };

    {
        std::lock_guard lock(mutex);
        state.phase = RecorderPhase::Recording;
        state.sinceMs = epoch();
        updateRecorderUi(state);
    }

    auto value = memory();
    print("recorder open: working set " + megabytes(value.workingSet) + ", private " + megabytes(value.privateBytes));
    if (HWND toolbar = FindWindowW(L"FileshipRecorder", nullptr); toolbar && flag) {
        RECT bounds{};
        GetClientRect(toolbar, &bounds);
        int middle = bounds.bottom / 2;
        for (int x = 0; x < bounds.right; x += 12) {
            PostMessageW(toolbar, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, middle));
            PostMessageW(toolbar, WM_LBUTTONUP, 0, MAKELPARAM(x, middle));
            Sleep(30);
        }
    }
    Sleep(seconds * 1000);

    {
        std::lock_guard lock(mutex);
        state.phase = RecorderPhase::Processing;
        updateRecorderUi(state);
    }
    Sleep(1000);
    hideRecorderUi();
    Sleep(200);
    return 0;
}


}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    if ((!console || console == INVALID_HANDLE_VALUE) && AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* stream = nullptr;
        freopen_s(&stream, "CONOUT$", "w", stdout);
    }

    winrt::init_apartment(winrt::apartment_type::multi_threaded);

    int count = 0;
    auto arguments = CommandLineToArgvW(GetCommandLineW(), &count);
    std::vector<std::wstring> args(arguments + 1, arguments + count);
    LocalFree(arguments);

    auto option = [&](std::wstring const& name, std::wstring const& fallback = L"") {
        for (size_t index = 0; index + 1 < args.size(); index++)
            if (args[index] == name) return args[index + 1];
        return fallback;
    };
    auto flag = [&](std::wstring const& name) { return std::find(args.begin(), args.end(), name) != args.end(); };

    std::filesystem::path resources = option(L"--resources", (executableDirectory() / L".." / L"host").wstring());
    initLog(std::filesystem::current_path() / L"overlay-demo.log");
    initI18n(resources / L"i18n");
    setLanguage(option(L"--language", L"en"));
    setUiResourcesPath(resources);

    auto preferences = executableDirectory() / L"preferences.json";
    setOverlayPreferencesStore({
        [preferences] {
            std::ifstream file(preferences, std::ios::binary);
            std::stringstream content;
            content << file.rdbuf();
            return content.str();
        },
        [preferences](std::string const& value) {
            std::ofstream file(preferences, std::ios::binary);
            file << value;
            print("saved preferences " + value);
        },
    });

    auto theme = option(L"--theme", L"dark") == L"light" ? ThemeMode::Light : ThemeMode::Dark;

    try {
        if (flag(L"--fixture")) return runFixture(option(L"--fixture"));
        if (flag(L"--recorder")) return runRecorder(theme, std::stoi(option(L"--seconds", L"4")), flag(L"--click"));

        auto purpose = parsePurpose(toUtf8(option(L"--purpose", L"capture")));
        auto output = std::filesystem::path(option(L"--out", (executableDirectory() / L"capture.png").wstring()));
        int repeat = std::stoi(option(L"--repeat", L"1"));
        int code = 0;
        for (int index = 0; index < repeat; index++)
            code = runOverlay(purpose, theme, output, option(L"--script"), flag(L"--synthetic"));
        return code;
    } catch (winrt::hresult_error const& error) {
        print("error: " + toUtf8(std::wstring(error.message())));
        return 1;
    }
}
