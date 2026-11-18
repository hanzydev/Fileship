#include "test.h"

#include <shared/utf8.h>

using namespace fileship;

TEST(utf8ConvertsBothWays) {
    CHECK_EQUAL(toWide(""), L"");
    CHECK_EQUAL(toWide("plain"), L"plain");
    CHECK_EQUAL(toWide("\xc3\xa9\xe2\x82\xac"), L"\xe9\x20ac");
    CHECK_EQUAL(toWide("\xf0\x9f\x98\x80"), L"\U0001F600");
    CHECK_EQUAL(toWide("a\xf0\x9f\x98\x80z"), L"a\U0001F600z");

    CHECK(toUtf8(L"") == "");
    CHECK(toUtf8(L"plain") == "plain");
    CHECK(toUtf8(L"\xe9\x20ac") == "\xc3\xa9\xe2\x82\xac");
    CHECK(toUtf8(L"\U0001F600") == "\xf0\x9f\x98\x80");
    CHECK(toUtf8(toWide("\xe4\xb8\xad\xe6\x96\x87 \xd1\x80\xd1\x83\xd1\x81")) ==
          "\xe4\xb8\xad\xe6\x96\x87 \xd1\x80\xd1\x83\xd1\x81");
}

TEST(utf8ReplacesInvalidBytes) {
    CHECK_EQUAL(toWide("a\xff"
                       "b"),
                L"a\xfffd"
                L"b");
    CHECK_EQUAL(toWide("a\xc3"), L"a\xfffd");
    CHECK_EQUAL(toWide("\xe2\x82"), L"\xfffd");
    CHECK_EQUAL(toWide("\xe2\x82z"), L"\xfffdz");
    CHECK_EQUAL(toWide("\x80"), L"\xfffd");
    CHECK_EQUAL(toWide("\xc0\xaf"), L"\xfffd\xfffd");
}
