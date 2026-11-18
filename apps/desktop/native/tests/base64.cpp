#include "test.h"

#include <shared/base64.h>

#include <string>

using namespace fileship;

namespace {

std::wstring encoded(std::string const& bytes) {
    return base64(bytes.data(), bytes.size());
}

std::string decoded(std::wstring const& text) {
    auto bytes = fromBase64(text);
    return std::string(bytes.begin(), bytes.end());
}

}

TEST(base64Encodes) {
    CHECK_EQUAL(encoded(""), L"");
    CHECK_EQUAL(encoded("f"), L"Zg==");
    CHECK_EQUAL(encoded("fo"), L"Zm8=");
    CHECK_EQUAL(encoded("foo"), L"Zm9v");
    CHECK_EQUAL(encoded("foobar"), L"Zm9vYmFy");
    CHECK_EQUAL(encoded("\xfb\xff"), L"+/8=");
    CHECK_EQUAL(encoded(std::string(1, '\0')), L"AA==");
}

TEST(base64Decodes) {
    CHECK(decoded(L"") == "");
    CHECK(decoded(L"Zg==") == "f");
    CHECK(decoded(L"Zm8=") == "fo");
    CHECK(decoded(L"Zm9v") == "foo");
    CHECK(decoded(L"Zm9vYmFy") == "foobar");
    CHECK(decoded(L"+/8=") == "\xfb\xff");
    CHECK(decoded(L"Zm9v\r\nYmFy\n") == "foobar");
    CHECK(fromBase64(L"AA==").size() == 1);
}

TEST(base64RejectsOtherCharacters) {
    CHECK(fromBase64(L"Zm9*").empty());
    CHECK(fromBase64(L"\xe9\xe9\xe9\xe9").empty());
}

TEST(base64RoundTripsEveryLength) {
    std::string bytes;

    for (int length = 0; length < 70; length++) {
        CHECK(decoded(encoded(bytes)) == bytes);
        bytes += static_cast<char>(length * 37 + 11);
    }
}
