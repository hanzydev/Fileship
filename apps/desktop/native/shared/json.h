#pragma once

#include "utf8.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#pragma push_macro("GetObject")
#undef GetObject

namespace fileship {

enum class JsonValueType { Null, Boolean, Number, String, Array, Object };

struct JsonError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct JsonNode;
class JsonArray;
class JsonObject;

class IJsonValue {
public:
    IJsonValue() = default;
    IJsonValue(std::nullptr_t) {}

    explicit operator bool() const {
        return node != nullptr;
    }

    JsonValueType ValueType() const;
    std::wstring GetString() const;
    double GetNumber() const;
    bool GetBoolean() const;
    JsonObject GetObject() const;
    JsonObject GetObjectW() const;
    JsonArray GetArray() const;
    std::wstring Stringify() const;

protected:
    explicit IJsonValue(std::shared_ptr<JsonNode> node) : node(std::move(node)) {}

    std::shared_ptr<JsonNode> node;

    friend class JsonArray;
    friend class JsonObject;
    friend class JsonValue;
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
    explicit JsonValue(std::shared_ptr<JsonNode> node) : IJsonValue(std::move(node)) {}

    friend class JsonArray;
    friend class JsonObject;
};

class JsonMember {
public:
    JsonMember(std::wstring key, IJsonValue value) : key(std::move(key)), value(std::move(value)) {}

    std::wstring const& Key() const {
        return key;
    }

    IJsonValue const& Value() const {
        return value;
    }

private:
    std::wstring key;
    IJsonValue value;
};

template <typename Item>
class JsonIterator {
public:
    JsonIterator() = default;
    explicit JsonIterator(std::vector<Item> items)
        : items(std::make_shared<std::vector<Item>>(std::move(items))) {}

    Item const& operator*() const {
        return (*items)[index];
    }

    JsonIterator& operator++() {
        index++;
        return *this;
    }

    bool operator!=(JsonIterator const&) const {
        return items && index < items->size();
    }

private:
    std::shared_ptr<std::vector<Item>> items;
    size_t index = 0;
};

class JsonObject : public IJsonValue {
public:
    JsonObject();
    JsonObject(std::nullptr_t) {}

    static JsonObject Parse(std::wstring_view text);
    static bool TryParse(std::wstring_view text, JsonObject& result);

    bool HasKey(std::wstring_view key) const;
    uint32_t Size() const;

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

    void SetNamedValue(std::wstring_view key, IJsonValue const& value);
    void Remove(std::wstring_view key);

    JsonIterator<JsonMember> begin() const;
    JsonIterator<JsonMember> end() const {
        return {};
    }

private:
    explicit JsonObject(std::shared_ptr<JsonNode> node) : IJsonValue(std::move(node)) {}

    std::shared_ptr<JsonNode> find(std::wstring_view key) const;

    friend class IJsonValue;
    friend class JsonArray;
};

class JsonArray : public IJsonValue {
public:
    JsonArray();
    JsonArray(std::nullptr_t) {}

    static bool TryParse(std::wstring_view text, JsonArray& result);

    uint32_t Size() const;
    IJsonValue GetAt(uint32_t index) const;
    std::wstring GetStringAt(uint32_t index) const;
    double GetNumberAt(uint32_t index) const;
    bool GetBooleanAt(uint32_t index) const;
    JsonObject GetObjectAt(uint32_t index) const;
    JsonArray GetArrayAt(uint32_t index) const;

    void Append(IJsonValue const& value);

    JsonIterator<IJsonValue> begin() const;
    JsonIterator<IJsonValue> end() const {
        return {};
    }

private:
    explicit JsonArray(std::shared_ptr<JsonNode> node) : IJsonValue(std::move(node)) {}

    friend class IJsonValue;
    friend class JsonObject;
};

inline JsonObject IJsonValue::GetObjectW() const {
    return GetObject();
}

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

inline std::string stringify(IJsonValue const& value) {
    return toUtf8(value.Stringify());
}

}

#pragma pop_macro("GetObject")
