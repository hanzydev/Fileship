#pragma once

#include <host/recording/command.h>

namespace fileship::host::recording {

struct OutputTarget {
    int adapter = 0;
    int index = 0;
    ui::ScreenRect bounds;
};

VideoInput ddaInput(VideoRequest const& request, OutputTarget const& output);
VideoInput gdiInput(VideoRequest const& request);

}
