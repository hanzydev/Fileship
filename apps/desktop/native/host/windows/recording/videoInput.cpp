#include "video.h"

namespace fileship::host::recording {

namespace {

std::wstring cursorFlag(VideoRequest const& request) {
    return request.showsCursor ? L"1" : L"0";
}

}

VideoInput ddaInput(VideoRequest const& request, OutputTarget const& output) {
    auto const& rect = request.rect;
    auto options = L"output_idx=" + std::to_wstring(output.index) + L":framerate=" + request.fps + L":draw_mouse=" +
                   cursorFlag(request) + L":offset_x=" + std::to_wstring(rect.x - output.bounds.x) + L":offset_y=" +
                   std::to_wstring(rect.y - output.bounds.y) + L":video_size=" + request.size;

    return {{L"-init_hw_device", L"d3d11va=dda:" + std::to_wstring(output.adapter), L"-filter_hw_device", L"dda"},
            L"ddagrab=" + options + L",showinfo=checksum=0,hwdownload,format=bgra,format=" + request.format + L"[v]",
            false};
}

VideoInput gdiInput(VideoRequest const& request) {
    auto const& rect = request.rect;

    return {{L"-f", L"gdigrab", L"-framerate", request.fps, L"-draw_mouse", cursorFlag(request), L"-offset_x",
             std::to_wstring(rect.x), L"-offset_y", std::to_wstring(rect.y), L"-video_size", request.size, L"-i",
             L"desktop"},
            L"[0:v]showinfo=checksum=0,fps=" + request.fps + L",format=" + request.format + L"[v]", true};
}

}
