#include "../test.h"

#include <host/harbor/paths.h>
#include <host/harbor/platform.h>
#include <shared/utf8.h>

using namespace fileship;
using namespace fileship::host::harbor;

TEST(harborNamesKeepWhatLinuxAllows) {
    CHECK_EQUAL(safeName(L"what?.txt", L"File"), L"what?.txt");
    CHECK_EQUAL(safeName(L"a:b*c|d\\e", L"File"), L"a:b*c|d\\e");
    CHECK_EQUAL(safeName(L"CON", L"File"), L"CON");
    CHECK_EQUAL(safeName(L"trailing.", L"File"), L"trailing.");
}

TEST(harborNamesDropSlashesAndControlCharacters) {
    CHECK_EQUAL(safeName(L"a/b", L"File"), L"ab");
    CHECK_EQUAL(safeName(L"a\nb\tc", L"File"), L"abc");
    CHECK_EQUAL(safeName(L"  spaced  ", L"File"), L"spaced");
}

TEST(harborNamesFallBackWhenNothingIsLeft) {
    CHECK_EQUAL(safeName(L"", L"File"), L"File");
    CHECK_EQUAL(safeName(L"///", L"Folder"), L"Folder");
    CHECK_EQUAL(safeName(L".", L"File"), L"File");
    CHECK_EQUAL(safeName(L"..", L"File"), L"File");
}

TEST(harborNamesFitAFileNameInBytes) {
    auto name = safeName(std::wstring(100, L'\x4e2d'), L"File");
    CHECK(name.size() == 66);
    CHECK(toUtf8(name).size() == 198);
    CHECK(safeName(std::wstring(300, L'a'), L"File").size() == 200);
}

TEST(harborPathsAreCaseSensitive) {
    CHECK_EQUAL(pathKey(L"Folder/File.TXT"), L"Folder/File.TXT");
    CHECK(SEPARATOR == L'/');
}
