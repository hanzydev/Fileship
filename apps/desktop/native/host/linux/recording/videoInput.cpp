#include "video.h"

namespace fileship::host::recording {

namespace {

std::wstring filterOf(VideoRequest const& request) {
    return L"[0:v]showinfo=checksum=0,fps=" + request.rate() + L",format=" + request.format + L"[v]";
}

}

VideoInput x11Input(VideoRequest const& request, std::wstring const& display) {
    auto const& rect = request.rect;

    return {{L"-f", L"x11grab", L"-framerate", request.rate(), L"-draw_mouse", request.cursor(), L"-video_size",
             request.size(), L"-i", display + L"+" + std::to_wstring(rect.x) + L"," + std::to_wstring(rect.y)},
            filterOf(request), true};
}

VideoInput framesInput(VideoRequest const& request, std::wstring const& pipe) {
    return {{L"-f", L"rawvideo", L"-pixel_format", L"bgr0", L"-video_size", request.size(), L"-framerate",
             request.rate(), L"-i", pipe},
            filterOf(request), true};
}

}
