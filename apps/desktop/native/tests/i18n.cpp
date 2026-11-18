#include "test.h"

#include <shared/i18n.h>

#include <filesystem>
#include <fstream>

using namespace fileship;

namespace {

void writeCatalog(std::filesystem::path const& folder, char const* code, char const* json) {
    std::ofstream(folder / (std::string(code) + ".json"), std::ios::binary) << json;
}

void useLanguage(std::wstring const& code) {
    static std::filesystem::path const folder = [] {
        auto path = std::filesystem::temp_directory_path() / "fileship-tests-i18n";
        std::filesystem::create_directories(path);

        writeCatalog(path, "en",
                     R"({"app":{"hello":"Hello {name}","files":"{count} file | {count} files",)"
                     R"("items":"none | one | many","literal":"mail{'@'}host","missing":"only english"}})");
        writeCatalog(path, "tr", R"({"app":{"hello":"Merhaba {name}","files":"{count} dosya"}})");
        writeCatalog(path, "fr", R"({"app":{"files":"{count} fichier | {count} fichiers"}})");
        writeCatalog(path, "ru",
                     R"({"app":{"files":"{count} f1 | {count} f2 | {count} f5","items":"z | a | b | c"}})");
        return path;
    }();

    initI18n(folder);
    setLanguage(L"en");
    setLanguage(code);
}

std::wstring files(long long count) {
    return t(L"app.files", {{L"count", std::to_wstring(count)}});
}

std::wstring items(long long count) {
    return t(L"app.items", {{L"count", std::to_wstring(count)}});
}

}

TEST(i18nFillsParameters) {
    useLanguage(L"en");

    CHECK_EQUAL(t(L"app.hello", {{L"name", L"Ada"}}), L"Hello Ada");
    CHECK_EQUAL(t(L"app.hello"), L"Hello {name}");
    CHECK_EQUAL(t(L"app.literal"), L"mail@host");
    CHECK_EQUAL(t(L"app.unknown"), L"app.unknown");
    CHECK_EQUAL(t(L"app"), L"app");
    CHECK_EQUAL(language(), L"en");
}

TEST(i18nFallsBackToEnglish) {
    useLanguage(L"tr");

    CHECK_EQUAL(t(L"app.hello", {{L"name", L"Ada"}}), L"Merhaba Ada");
    CHECK_EQUAL(t(L"app.missing"), L"only english");
    CHECK_EQUAL(files(5), L"5 dosya");
    CHECK_EQUAL(language(), L"tr");
}

TEST(i18nPicksEnglishPlurals) {
    useLanguage(L"en");

    CHECK_EQUAL(files(0), L"0 files");
    CHECK_EQUAL(files(1), L"1 file");
    CHECK_EQUAL(files(2), L"2 files");
    CHECK_EQUAL(items(0), L"none");
    CHECK_EQUAL(items(1), L"one");
    CHECK_EQUAL(items(7), L"many");
    CHECK_EQUAL(t(L"app.files"), L"{count} file");
}

TEST(i18nPicksFrenchPlurals) {
    useLanguage(L"fr");

    CHECK_EQUAL(files(0), L"0 fichier");
    CHECK_EQUAL(files(1), L"1 fichier");
    CHECK_EQUAL(files(2), L"2 fichiers");
}

TEST(i18nPicksRussianPlurals) {
    useLanguage(L"ru");

    CHECK_EQUAL(files(1), L"1 f1");
    CHECK_EQUAL(files(2), L"2 f2");
    CHECK_EQUAL(files(5), L"5 f5");
    CHECK_EQUAL(files(11), L"11 f5");
    CHECK_EQUAL(files(21), L"21 f1");
    CHECK_EQUAL(files(22), L"22 f2");
    CHECK_EQUAL(files(112), L"112 f5");
    CHECK_EQUAL(items(0), L"z");
    CHECK_EQUAL(items(1), L"a");
    CHECK_EQUAL(items(3), L"b");
    CHECK_EQUAL(items(5), L"c");
}

TEST(i18nResolvesTheLanguageSetting) {
    CHECK_EQUAL(resolveLanguage(L"de"), L"de");

    auto system = resolveLanguage(L"system");
    auto supported = languages();
    CHECK(std::find(supported.begin(), supported.end(), system) != supported.end());
}
