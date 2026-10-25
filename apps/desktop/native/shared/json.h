#pragma once

#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>

#include "utf8.h"

#include <optional>
#include <string>

namespace fileship {

using winrt::Windows::Data::Json::JsonArray;
using winrt::Windows::Data::Json::JsonObject;
using winrt::Windows::Data::Json::JsonValue;
using winrt::Windows::Data::Json::JsonValueType;

inline std::optional<JsonObject> parseObject(std::string_view text) {
    JsonObject object{nullptr};
    if (!JsonObject::TryParse(toWide(text), object)) return std::nullopt;
    return object;
}

inline std::optional<JsonArray> parseArray(std::string_view text) {
    JsonArray array{nullptr};
    if (!JsonArray::TryParse(toWide(text), array)) return std::nullopt;
    return array;
}

inline std::string stringify(JsonObject const& object) {
    return toUtf8(std::wstring(object.Stringify()));
}

inline std::string stringify(JsonArray const& array) {
    return toUtf8(std::wstring(array.Stringify()));
}

inline std::string stringify(JsonValue const& value) {
    return toUtf8(std::wstring(value.Stringify()));
}

}
