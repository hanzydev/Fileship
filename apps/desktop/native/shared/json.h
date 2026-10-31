#pragma once

#ifdef _WIN32
#include <windows.h>
#undef GetObject
#endif

#include "utf8.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace fileship {

enum class JsonValueType { Null, Boolean, Number, String, Array, Object };

class JsonError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class JsonObject;
class JsonArray;
struct JsonNode;

class IJsonValue {
public:
    IJsonValue() = default;
    IJsonValue(std::nullptr_t) {}

    JsonValueType ValueType() const;
    std::wstring GetString() const;
    double GetNumber() const;
    bool GetBoolean() const;
    JsonObject GetObject() const;
    JsonArray GetArray() const;
    std::wstring Stringify() const;

    explicit operator bool() const {
        return node != nullptr;
    }

    bool operator==(std::nullptr_t) const {
        return node == nullptr;
    }

protected:
    explicit IJsonValue(std::shared_ptr<JsonNode> value) : node(std::move(value)) {}

    JsonNode& checked() const;
    JsonNode& checked(JsonValueType type) const;

    std::shared_ptr<JsonNode> node;

    friend class JsonValue;
    friend class JsonObject;
    friend class JsonArray;
    friend struct JsonAccess;
};

class JsonValue : public IJsonValue {
public:
    JsonValue(std::nullptr_t) {}

    static JsonValue CreateNullValue();
    static JsonValue CreateBooleanValue(bool value);
    static JsonValue CreateNumberValue(double value);
    static JsonValue CreateStringValue(std::wstring_view value);
    static JsonValue Parse(std::wstring_view text);
    static bool TryParse(std::wstring_view text, JsonValue& result);

private:
    explicit JsonValue(std::shared_ptr<JsonNode> value) : IJsonValue(std::move(value)) {}

    friend class JsonObject;
};

class JsonPair {
public:
    JsonPair(std::wstring const& key, IJsonValue const& value) : key(&key), value(&value) {}

    std::wstring const& Key() const {
        return *key;
    }

    IJsonValue Value() const {
        return *value;
    }

private:
    std::wstring const* key;
    IJsonValue const* value;
};

class JsonObject : public IJsonValue {
public:
    using Members = std::vector<std::pair<std::wstring, IJsonValue>>;

    class Iterator {
    public:
        explicit Iterator(Members::const_iterator position) : position(position) {}

        JsonPair operator*() const {
            return {position->first, position->second};
        }

        Iterator& operator++() {
            ++position;
            return *this;
        }

        bool operator==(Iterator const& other) const = default;

    private:
        Members::const_iterator position;
    };

    JsonObject();
    JsonObject(std::nullptr_t) {}

    static JsonObject Parse(std::wstring_view text);
    static bool TryParse(std::wstring_view text, JsonObject& result);

    uint32_t Size() const;
    bool HasKey(std::wstring_view key) const;
    void SetNamedValue(std::wstring_view key, IJsonValue const& value);
    bool Remove(std::wstring_view key);

    JsonValue GetNamedValue(std::wstring_view key) const;
    JsonValue GetNamedValue(std::wstring_view key, JsonValue const& fallback) const;
    std::wstring GetNamedString(std::wstring_view key) const;
    std::wstring GetNamedString(std::wstring_view key, std::wstring_view fallback) const;
    double GetNamedNumber(std::wstring_view key) const;
    double GetNamedNumber(std::wstring_view key, double fallback) const;
    bool GetNamedBoolean(std::wstring_view key) const;
    bool GetNamedBoolean(std::wstring_view key, bool fallback) const;
    JsonObject GetNamedObject(std::wstring_view key) const;
    JsonObject GetNamedObject(std::wstring_view key, JsonObject const& fallback) const;
    JsonArray GetNamedArray(std::wstring_view key) const;
    JsonArray GetNamedArray(std::wstring_view key, JsonArray const& fallback) const;

    Iterator begin() const;
    Iterator end() const;

private:
    explicit JsonObject(std::shared_ptr<JsonNode> value) : IJsonValue(std::move(value)) {}

    IJsonValue const* find(std::wstring_view key) const;
    IJsonValue const& lookup(std::wstring_view key) const;

    friend class IJsonValue;
};

class JsonArray : public IJsonValue {
public:
    using Items = std::vector<IJsonValue>;

    JsonArray();
    JsonArray(std::nullptr_t) {}

    static JsonArray Parse(std::wstring_view text);
    static bool TryParse(std::wstring_view text, JsonArray& result);

    uint32_t Size() const;
    void Append(IJsonValue const& value);
    IJsonValue GetAt(uint32_t index) const;
    std::wstring GetStringAt(uint32_t index) const;
    double GetNumberAt(uint32_t index) const;
    bool GetBooleanAt(uint32_t index) const;
    JsonObject GetObjectAt(uint32_t index) const;
    JsonArray GetArrayAt(uint32_t index) const;

    Items::const_iterator begin() const;
    Items::const_iterator end() const;

private:
    explicit JsonArray(std::shared_ptr<JsonNode> value) : IJsonValue(std::move(value)) {}

    friend class IJsonValue;
};

IJsonValue parseJson(std::string_view text);

inline std::optional<JsonObject> parseObject(std::string_view text) {
    try {
        auto value = parseJson(text);
        if (value.ValueType() != JsonValueType::Object) return std::nullopt;
        return value.GetObject();
    } catch (JsonError const&) {
        return std::nullopt;
    }
}

inline std::optional<JsonArray> parseArray(std::string_view text) {
    try {
        auto value = parseJson(text);
        if (value.ValueType() != JsonValueType::Array) return std::nullopt;
        return value.GetArray();
    } catch (JsonError const&) {
        return std::nullopt;
    }
}

std::string stringify(IJsonValue const& value);

}
