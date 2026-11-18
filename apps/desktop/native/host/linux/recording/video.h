#pragma once

#include <host/recording/command.h>

namespace fileship::host::recording {

VideoInput x11Input(VideoRequest const& request, std::wstring const& display);
VideoInput framesInput(VideoRequest const& request, std::wstring const& pipe);

}
