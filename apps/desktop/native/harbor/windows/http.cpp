#include "http.h"

namespace harbor {

namespace {

constexpr int TIMEOUT_MS = 60000;

fileship::HttpEndpoint endpoint() {
    return {config().instanceUrl, token(), config().userAgent, {}};
}

}

HttpStream::HttpStream(std::wstring const& path, std::wstring const& headers)
    : fileship::HttpStream(endpoint(), {L"GET", path, headers, {}, TIMEOUT_MS}) {}

fileship::HttpResponse httpGet(std::wstring const& path) {
    return fileship::httpSend(endpoint(), {L"GET", path, {}, {}, TIMEOUT_MS});
}

}
