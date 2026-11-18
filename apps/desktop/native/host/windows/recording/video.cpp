#include "video.h"

#include <host/recording/platform.h>
#include <shared/log.h>

#include <windows.h>

#include <dxgi1_2.h>
#include <winrt/base.h>

namespace fileship::host::recording {

namespace {

bool contains(ui::ScreenRect const& outer, ui::ScreenRect const& inner) {
    return inner.x >= outer.x && inner.y >= outer.y && inner.x + inner.width <= outer.x + outer.width &&
           inner.y + inner.height <= outer.y + outer.height;
}

std::optional<OutputTarget> findOutput(ui::ScreenRect const& rect) {
    winrt::com_ptr<IDXGIFactory1> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(factory.put())))) {
        logWarn(L"Could not list the displays");
        return std::nullopt;
    }

    winrt::com_ptr<IDXGIAdapter1> adapter;
    for (UINT adapterIndex = 0; factory->EnumAdapters1(adapterIndex, adapter.put()) != DXGI_ERROR_NOT_FOUND;
         adapterIndex++) {
        winrt::com_ptr<IDXGIOutput> output;

        for (UINT outputIndex = 0; adapter->EnumOutputs(outputIndex, output.put()) != DXGI_ERROR_NOT_FOUND;
             outputIndex++) {
            DXGI_OUTPUT_DESC desc{};

            if (SUCCEEDED(output->GetDesc(&desc))) {
                auto const& bounds = desc.DesktopCoordinates;
                ui::ScreenRect outputRect{bounds.left, bounds.top, bounds.right - bounds.left,
                                          bounds.bottom - bounds.top};
                if (contains(outputRect, rect)) {
                    return OutputTarget{static_cast<int>(adapterIndex), static_cast<int>(outputIndex), outputRect};
                }
            }

            output = nullptr;
        }

        adapter = nullptr;
    }

    return std::nullopt;
}

ui::ScreenRect primaryBounds() {
    MONITORINFO info{sizeof(info)};
    GetMonitorInfoW(MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY), &info);
    auto const& rect = info.rcMonitor;
    return {rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top};
}

bool prefersDda(JsonObject const& settings) {
    return text(section(settings, L"recording"), L"source", L"auto") != L"gdigrab";
}

class Capture : public VideoCapture {
public:
    explicit Capture(VideoInput value) : video(std::move(value)) {}

    VideoInput const& input() const override {
        return video;
    }

    void stop() override {}

private:
    VideoInput video;
};

}

std::unique_ptr<VideoCapture> VideoCapture::start(JsonObject const& settings, VideoRequest const& request,
                                                  Ffmpeg const& ffmpeg) {
    auto output = findOutput(request.rect);
    bool useDda = prefersDda(settings) && ffmpeg.ddagrab && output;
    return std::make_unique<Capture>(useDda ? ddaInput(request, *output) : gdiInput(request));
}

VideoInput previewVideoInput(JsonObject const& settings, std::wstring const& codec,
                             std::optional<Ffmpeg> const& ffmpeg) {
    auto bounds = primaryBounds();
    auto request = videoRequest(settings, L"video", codec, bounds);
    bool useDda = prefersDda(settings) && (!ffmpeg || ffmpeg->ddagrab);
    return useDda ? ddaInput(request, findOutput(bounds).value_or(OutputTarget{0, 0, bounds})) : gdiInput(request);
}

void showRecorderControls(ui::ScreenRect const& area, ui::ThemeMode theme, ui::RecorderUiCallbacks callbacks,
                          bool showBorder) {
    ui::showRecorderUi(area, theme, std::move(callbacks), showBorder);
}

void updateRecorderControls(ui::RecorderViewState const& state) {
    ui::updateRecorderUi(state);
}

void hideRecorderControls() {
    ui::hideRecorderUi();
}

}
