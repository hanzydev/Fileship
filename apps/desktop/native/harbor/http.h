#pragma once

#include "common.h"

#include "../shared/http.h"

namespace harbor {

struct HttpStream : fileship::HttpStream {
    HttpStream(std::wstring const& path, std::wstring const& headers = {});
};

fileship::HttpResponse httpGet(std::wstring const& path);

}
