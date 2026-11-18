#include "test.h"

#include <shared/url.h>

using namespace fileship;

namespace {

std::wstring parsed(std::wstring const& text) {
    auto url = parseHttpUrl(text);
    if (!url) return L"invalid";

    return std::wstring(url->isSecure ? L"https " : L"http ") + url->host + L" " + std::to_wstring(url->port);
}

}

TEST(urlReadsSchemeHostAndPort) {
    CHECK_EQUAL(parsed(L"https://example.com"), L"https example.com 443");
    CHECK_EQUAL(parsed(L"http://Example.COM:80/"), L"http Example.COM 80");
    CHECK_EQUAL(parsed(L"HTTPS://EXAMPLE.com/Path"), L"https EXAMPLE.com 443");
    CHECK_EQUAL(parsed(L"https://files.example.com:8443/dashboard?x=1#y"), L"https files.example.com 8443");
    CHECK_EQUAL(parsed(L"http://localhost:3000"), L"http localhost 3000");
    CHECK_EQUAL(parsed(L"http://example.com:443"), L"http example.com 443");
    CHECK_EQUAL(parsed(L"https://example.com:0"), L"https example.com 0");
    CHECK_EQUAL(parsed(L"https://example.com:65535"), L"https example.com 65535");
    CHECK_EQUAL(parsed(L"https://example.com:"), L"https example.com 443");
    CHECK_EQUAL(parsed(L"https://example.com?x=1"), L"https example.com 443");
    CHECK_EQUAL(parsed(L"https://example.com#frag"), L"https example.com 443");
    CHECK_EQUAL(parsed(L"http://127.0.0.1:3000"), L"http 127.0.0.1 3000");
    CHECK_EQUAL(parsed(L"https://sso.localhost:3000/login"), L"https sso.localhost 3000");
    CHECK_EQUAL(parsed(L"https://\xfcni.example"), L"https \xfcni.example 443");
}

TEST(urlDropsUserInfoAndKeepsIpv6Brackets) {
    CHECK_EQUAL(parsed(L"https://user:pass@example.com:8080/x"), L"https example.com 8080");
    CHECK_EQUAL(parsed(L"https://user@example.com"), L"https example.com 443");
    CHECK_EQUAL(parsed(L"http://[::1]:3000/"), L"http [::1] 3000");
    CHECK_EQUAL(parsed(L"http://[fe80::1]"), L"http [fe80::1] 80");
}

TEST(urlRejectsWhatCannotBeAnAddress) {
    CHECK_EQUAL(parsed(L""), L"invalid");
    CHECK_EQUAL(parsed(L"example.com"), L"invalid");
    CHECK_EQUAL(parsed(L"ftp://example.com"), L"invalid");
    CHECK_EQUAL(parsed(L"https://"), L"invalid");
    CHECK_EQUAL(parsed(L"https:///path"), L"invalid");
    CHECK_EQUAL(parsed(L"https://:8080"), L"invalid");
    CHECK_EQUAL(parsed(L"https://example.com:abc"), L"invalid");
    CHECK_EQUAL(parsed(L"https://example.com:65536"), L"invalid");
    CHECK_EQUAL(parsed(L"https://example.com:99999"), L"invalid");
}

TEST(urlTreatsALaterSchemeAsHostAndPort) {
    CHECK_EQUAL(parsed(L"https://ftp://x"), L"https ftp 443");
}
