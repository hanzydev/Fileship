#include "test.h"

#include <host/updateFeed.h>

using namespace fileship::host;

namespace {

char const* const WINDOWS_FEED = "version: 2.6.1\r\n"
                                 "files:\r\n"
                                 "  - url: Fileship-Setup-2.6.1.exe\r\n"
                                 "    sha512: d2luZG93cw==\r\n"
                                 "    size: 98765432\r\n"
                                 "path: Fileship-Setup-2.6.1.exe\r\n"
                                 "sha512: d2luZG93cw==\r\n"
                                 "releaseDate: '2026-11-10T10:00:00.000Z'\r\n";

char const* const LINUX_FEED = "version: 2.7.0\n"
                               "files:\n"
                               "  - url: Fileship-2.7.0.deb\n"
                               "    sha512: ZGVi\n"
                               "    size: 100\n"
                               "  - url: Fileship-2.7.0.rpm\n"
                               "    sha512: cnBt\n"
                               "    size: 200\n"
                               "path: Fileship-2.7.0.deb\n"
                               "sha512: ZGVi\n"
                               "releaseDate: '2026-11-19T10:00:00.000Z'\n";

bool isNewerText(wchar_t const* remote, wchar_t const* local) {
    return isNewer(*parseVersion(remote), *parseVersion(local));
}

}

TEST(updateFeedReadsTheFirstFile) {
    auto release = parseRelease(WINDOWS_FEED, L"");
    CHECK_EQUAL(release.version, L"2.6.1");
    CHECK_EQUAL(release.url, L"Fileship-Setup-2.6.1.exe");
    CHECK_EQUAL(release.sha512, L"d2luZG93cw==");
    CHECK(release.size == 98765432);
}

TEST(updateFeedPicksThePackageOfTheSystem) {
    auto deb = parseRelease(LINUX_FEED, L".deb");
    CHECK_EQUAL(deb.url, L"Fileship-2.7.0.deb");
    CHECK_EQUAL(deb.sha512, L"ZGVi");
    CHECK(deb.size == 100);

    auto rpm = parseRelease(LINUX_FEED, L".rpm");
    CHECK_EQUAL(rpm.version, L"2.7.0");
    CHECK_EQUAL(rpm.url, L"Fileship-2.7.0.rpm");
    CHECK_EQUAL(rpm.sha512, L"cnBt");
    CHECK(rpm.size == 200);
}

TEST(updateFeedHasNothingForAnotherPackage) {
    auto release = parseRelease("version: 2.7.0\nfiles:\n  - url: Fileship-2.7.0.deb\n    sha512: ZGVi\n    size: 100\n"
                                "path: Fileship-2.7.0.deb\nsha512: ZGVi\n",
                                L".rpm");
    CHECK_EQUAL(release.version, L"2.7.0");
    CHECK_EQUAL(release.url, L"");
    CHECK_EQUAL(release.sha512, L"");
}

TEST(updateFeedFallsBackToTheTopLevelFile) {
    auto release = parseRelease("version: \"1.2.3\"\npath: 'Setup 1.2.3.exe'\nsha512: dG9w\n", L"");
    CHECK_EQUAL(release.version, L"1.2.3");
    CHECK_EQUAL(release.url, L"Setup 1.2.3.exe");
    CHECK_EQUAL(release.sha512, L"dG9w");
}

TEST(versionsAreParsed) {
    CHECK(parseVersion(L"2.6.1").has_value());
    CHECK(parseVersion(L"v2.6.1").has_value());
    CHECK(parseVersion(L"2.6.1-beta.1+build").has_value());
    CHECK(!parseVersion(L"").has_value());
    CHECK(!parseVersion(L"latest").has_value());
    CHECK(!parseVersion(L"2.6").has_value());
}

TEST(versionsCompareLikeSemver) {
    CHECK(isNewerText(L"2.7.0", L"2.6.9"));
    CHECK(isNewerText(L"2.10.0", L"2.9.0"));
    CHECK(isNewerText(L"3.0.0", L"2.99.99"));
    CHECK(!isNewerText(L"2.6.1", L"2.6.1"));
    CHECK(!isNewerText(L"2.6.0", L"2.6.1"));
    CHECK(isNewerText(L"2.6.1", L"2.6.1-beta.1"));
    CHECK(!isNewerText(L"2.6.1-beta.1", L"2.6.1"));
    CHECK(isNewerText(L"2.6.1-beta.2", L"2.6.1-beta.1"));
}
