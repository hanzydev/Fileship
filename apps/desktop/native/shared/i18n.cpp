#include "i18n.h"

#include "json.h"
#include "log.h"

#include <cstdlib>
#include <fstream>
#include <mutex>
#include <optional>
#include <regex>
#include <sstream>

namespace fileship {

namespace {

std::mutex mutex;
std::filesystem::path catalogDirectory;
std::wstring currentLanguage = L"en";
JsonObject current{nullptr};
JsonObject fallback{nullptr};

JsonObject loadCatalog(std::wstring const& code) {
    std::ifstream file(catalogDirectory / (code + L".json"), std::ios::binary);
    std::stringstream content;
    content << file.rdbuf();

    if (auto object = parseObject(content.str())) return *object;

    logWarn(L"Could not load the " + code + L" messages");
    return JsonObject();
}

std::optional<std::wstring> lookup(JsonObject const& catalog, std::wstring_view key) {
    if (!catalog) return std::nullopt;

    JsonObject node = catalog;
    size_t start = 0;

    while (true) {
        size_t dot = key.find(L'.', start);
        std::wstring part(key.substr(start, dot == std::wstring_view::npos ? key.size() - start : dot - start));
        if (!node.HasKey(part)) return std::nullopt;

        auto value = node.GetNamedValue(part);
        if (dot == std::wstring_view::npos) {
            if (value.ValueType() != JsonValueType::String) return std::nullopt;
            return std::wstring(value.GetString());
        }

        if (value.ValueType() != JsonValueType::Object) return std::nullopt;
        node = node.GetNamedObject(part);
        start = dot + 1;
    }
}

size_t russianForm(long long count) {
    long long ones = count % 10;
    long long tens = count % 100;

    if (ones == 1 && tens != 11) return 0;
    return ones >= 2 && ones <= 4 && (tens < 12 || tens > 14) ? 1 : 2;
}

std::wstring choose(std::wstring const& message, std::optional<long long> count) {
    std::vector<std::wstring> choices;
    size_t start = 0;

    while (true) {
        size_t separator = message.find(L" | ", start);
        choices.push_back(message.substr(start, separator == std::wstring::npos ? std::wstring::npos
                                                                                : separator - start));
        if (separator == std::wstring::npos) break;
        start = separator + 3;
    }

    if (!count || choices.size() == 1) return choices[0];

    long long number = std::llabs(*count);
    if (currentLanguage == L"ru" && choices.size() > 2) {
        size_t form = russianForm(number);
        return choices.size() == 4 ? choices[number ? form + 1 : 0] : choices[form];
    }

    if (choices.size() == 2) {
        return choices[currentLanguage == L"fr" ? number > 1 : number != 1];
    }

    return choices[static_cast<size_t>(std::min<long long>(number, 2))];
}

}

void initI18n(std::filesystem::path const& directory) {
    std::lock_guard lock(mutex);
    catalogDirectory = directory;
    fallback = loadCatalog(L"en");
    current = fallback;
}

std::vector<std::wstring> languages() {
    return {L"en", L"tr", L"de", L"es", L"fr", L"it", L"pt", L"ru", L"az", L"ja", L"ko", L"zh"};
}

std::wstring resolveLanguage(std::wstring const& setting) {
    auto supported = languages();
    if (setting != L"system") return setting;

    ULONG count = 0;
    ULONG size = 0;
    GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &count, nullptr, &size);

    std::wstring buffer(size, L'\0');
    if (!GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &count, buffer.data(), &size)) return L"en";

    std::wstring first(buffer.c_str());
    std::wstring code = first.substr(0, first.find(L'-'));
    for (auto& character : code) character = static_cast<wchar_t>(towlower(character));

    return std::find(supported.begin(), supported.end(), code) != supported.end() ? code : L"en";
}

bool setLanguage(std::wstring const& code) {
    std::lock_guard lock(mutex);
    if (code == currentLanguage) return false;

    currentLanguage = code;
    current = code == L"en" ? fallback : loadCatalog(code);
    return true;
}

std::wstring language() {
    std::lock_guard lock(mutex);
    return currentLanguage;
}

std::wstring t(std::wstring_view key, I18nParams const& params) {
    std::lock_guard lock(mutex);

    auto message = lookup(current, key);
    if (!message) message = lookup(fallback, key);
    if (!message) return std::wstring(key);

    std::optional<long long> count;
    for (auto const& [name, value] : params) {
        if (name != L"count") continue;

        wchar_t* end = nullptr;
        long long number = wcstoll(value.c_str(), &end, 10);
        if (end && !*end) count = number;
    }

    static std::wregex const literal(LR"(\{'([^']*)'\})");
    static std::wregex const parameter(LR"(\{(\w+)\})");

    std::wstring chosen = choose(*message, count);
    std::wstring result;
    std::wsmatch match;
    auto begin = chosen.cbegin();

    while (std::regex_search(begin, chosen.cend(), match, parameter)) {
        result.append(begin, match[0].first);

        auto found = std::find_if(params.begin(), params.end(),
                                  [&](auto const& item) { return item.first == match[1].str(); });
        result += found == params.end() ? match[0].str() : found->second;
        begin = match[0].second;
    }

    result.append(begin, chosen.cend());
    return std::regex_replace(result, literal, L"$1");
}

}
