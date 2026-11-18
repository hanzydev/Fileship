#include "fifo.h"
#include "video.h"

#include <core/linux/screen.h>
#include <host/recording/platform.h>
#include <shared/utf8.h>

#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <thread>

namespace fileship::host::recording {

namespace {

std::atomic<int> nextPipe = 0;

std::wstring display() {
    return toWide(std::getenv("DISPLAY"));
}

class X11Capture : public VideoCapture {
public:
    explicit X11Capture(VideoRequest const& request) : video(x11Input(request, display())) {}

    VideoInput const& input() const override {
        return video;
    }

    void stop() override {}

private:
    VideoInput video;
};

class FrameCapture : public VideoCapture {
public:
    explicit FrameCapture(VideoRequest const& request)
        : frames({request.rect.x, request.rect.y, request.width, request.height}, request.showsCursor),
          path(fifoPath("video-" + std::to_string(getpid()) + "-" + std::to_string(nextPipe++))),
          fifo(path),
          video(framesInput(request, toWide(path))),
          frame(static_cast<size_t>(request.width) * request.height * 4),
          period(std::chrono::nanoseconds(1'000'000'000 / request.fps)) {
        thread = std::thread([this] { run(); });
    }

    ~FrameCapture() override {
        stop();
    }

    VideoInput const& input() const override {
        return video;
    }

    void stop() override {
        fifo.cancel();
        if (thread.joinable()) thread.join();
    }

private:
    void run() {
        if (!fifo.open()) return;

        auto next = std::chrono::steady_clock::now();
        while (frames.copy(frame.data()) ? fifo.write(reinterpret_cast<char const*>(frame.data()), frame.size())
                                         : fifo.wait(5)) {
            next = std::max(next + period, std::chrono::steady_clock::now() - period * 30);
            std::this_thread::sleep_until(next);
        }

        fifo.close();
    }

    core::wayland::FrameStream frames;
    std::string path;
    Fifo fifo;
    VideoInput video;
    std::vector<uint8_t> frame;
    std::chrono::nanoseconds period;
    std::thread thread;
};

}

std::unique_ptr<VideoCapture> VideoCapture::start(JsonObject const&, VideoRequest const& request, Ffmpeg const&) {
    if (core::isWayland()) return std::make_unique<FrameCapture>(request);
    return std::make_unique<X11Capture>(request);
}

VideoInput previewVideoInput(JsonObject const& settings, std::wstring const& codec, std::optional<Ffmpeg> const&) {
    if (core::isWayland())
        return framesInput(videoRequest(settings, L"video", codec, {0, 0, 1920, 1080}), toWide(fifoPath("video")));

    auto screen = core::virtualScreen();
    return x11Input(videoRequest(settings, L"video", codec, {screen.x, screen.y, screen.width, screen.height}),
                    display());
}

}
