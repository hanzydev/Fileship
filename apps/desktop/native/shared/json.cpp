#include "json.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <mutex>
#include <utility>

#undef GetObject

namespace fileship {

struct JsonNode {
    JsonValueType type = JsonValueType::Null;
    bool boolean = false;
    double number = 0;
    std::wstring string;
    std::vector<std::pair<std::wstring, std::shared_ptr<JsonNode>>> members;
    std::vector<std::shared_ptr<JsonNode>> items;
    std::mutex mutex;
};

namespace {

using Node = std::shared_ptr<JsonNode>;

constexpr int MAX_DEPTH = 512;

Node makeNode(JsonValueType type) {
    auto node = std::make_shared<JsonNode>();
    node->type = type;
    return node;
}

JsonNode& expect(Node const& node, JsonValueType type) {
    if (node->type != type) throw JsonError("The JSON value has another type");
    return *node;
}

void writeString(std::wstring& out, std::wstring_view text) {
    static wchar_t const HEX[] = L"0123456789ABCDEF";

    out += L'"';

    for (auto character : text) {
        auto code = static_cast<uint32_t>(character);

        switch (character) {
        case L'"':
            out += L'\\';
            out += L'"';
            break;
        case L'\\':
            out += L'\\';
            out += L'\\';
            break;
        case L'\b':
            out += L'\\';
            out += L'b';
            break;
        case L'\f':
            out += L'\\';
            out += L'f';
            break;
        case L'\n':
            out += L'\\';
            out += L'n';
            break;
        case L'\r':
            out += L'\\';
            out += L'r';
            break;
        case L'\t':
            out += L'\\';
            out += L't';
            break;
        default:
            if (code >= 0x20) {
                out += character;
                break;
            }

            out += L'\\';
            out += L"u00";
            out += HEX[code >> 4];
            out += HEX[code & 15];
        }
    }

    out += L'"';
}

void writeNumber(std::wstring& out, double number) {
    if (!std::isfinite(number)) {
        out += L"null";
        return;
    }

    char buffer[32];
    auto format = [&](int precision) {
        auto end = std::to_chars(buffer, buffer + sizeof(buffer), number, std::chars_format::general, precision).ptr;
        return std::string_view(buffer, static_cast<size_t>(end - buffer));
    };

    auto digits = format(15);
    double parsed = 0;
    std::from_chars(digits.data(), digits.data() + digits.size(), parsed);
    if (parsed != number) digits = format(17);

    auto exponent = digits.find('e');
    out.append(digits.begin(), digits.begin() + static_cast<ptrdiff_t>(std::min(exponent, digits.size())));
    if (exponent == std::string_view::npos) return;

    out += L'E';
    out += static_cast<wchar_t>(digits[exponent + 1]);

    auto power = digits.substr(exponent + 2);
    power.remove_prefix(std::min(power.find_first_not_of('0'), power.size() - 1));
    out.append(power.begin(), power.end());
}

void write(std::wstring& out, Node const& node) {
    switch (node->type) {
    case JsonValueType::Null:
        out += L"null";
        return;
    case JsonValueType::Boolean:
        out += node->boolean ? L"true" : L"false";
        return;
    case JsonValueType::Number:
        writeNumber(out, node->number);
        return;
    case JsonValueType::String:
        writeString(out, node->string);
        return;
    case JsonValueType::Array: {
        std::vector<Node> items;
        {
            std::lock_guard lock(node->mutex);
            items = node->items;
        }

        out += L'[';
        for (size_t index = 0; index < items.size(); index++) {
            if (index) out += L',';
            write(out, items[index]);
        }
        out += L']';
        return;
    }
    case JsonValueType::Object: {
        std::vector<std::pair<std::wstring, Node>> members;
        {
            std::lock_guard lock(node->mutex);
            members = node->members;
        }

        out += L'{';
        for (size_t index = 0; index < members.size(); index++) {
            if (index) out += L',';
            writeString(out, members[index].first);
            out += L':';
            write(out, members[index].second);
        }
        out += L'}';
        return;
    }
    }
}

class Parser {
public:
    explicit Parser(std::wstring_view text) : text(text) {}

    Node parse() {
        skipSpace();
        auto node = value();
        skipSpace();
        if (position != text.size()) fail();
        return node;
    }

private:
    [[noreturn]] void fail() const {
        throw JsonError("Invalid JSON at character " + std::to_string(position));
    }

    wchar_t peek() const {
        return position < text.size() ? text[position] : L'\0';
    }

    void skipSpace() {
        while (peek() == L' ' || peek() == L'\t' || peek() == L'\n' || peek() == L'\r') position++;
    }

    void literal(std::wstring_view word) {
        if (text.substr(position, word.size()) != word) fail();
        position += word.size();
    }

    Node value() {
        switch (peek()) {
        case L'{':
            return object();
        case L'[':
            return array();
        case L'"': {
            auto node = makeNode(JsonValueType::String);
            node->string = string();
            return node;
        }
        case L't': {
            literal(L"true");
            auto node = makeNode(JsonValueType::Boolean);
            node->boolean = true;
            return node;
        }
        case L'f':
            literal(L"false");
            return makeNode(JsonValueType::Boolean);
        case L'n':
            literal(L"null");
            return makeNode(JsonValueType::Null);
        default:
            return number();
        }
    }

    Node object() {
        if (++depth > MAX_DEPTH) fail();

        auto node = makeNode(JsonValueType::Object);
        position++;
        skipSpace();

        if (peek() == L'}') {
            position++;
            depth--;
            return node;
        }

        while (true) {
            skipSpace();
            if (peek() != L'"') fail();
            auto key = string();

            skipSpace();
            if (peek() != L':') fail();
            position++;
            skipSpace();

            auto child = value();
            auto existing = std::find_if(node->members.begin(), node->members.end(),
                                         [&](auto const& member) { return member.first == key; });
            if (existing == node->members.end()) node->members.emplace_back(std::move(key), std::move(child));
            else existing->second = std::move(child);

            skipSpace();
            if (peek() == L'}') break;
            if (peek() != L',') fail();
            position++;
        }

        position++;
        depth--;
        return node;
    }

    Node array() {
        if (++depth > MAX_DEPTH) fail();

        auto node = makeNode(JsonValueType::Array);
        position++;
        skipSpace();

        if (peek() == L']') {
            position++;
            depth--;
            return node;
        }

        while (true) {
            skipSpace();
            node->items.push_back(value());

            skipSpace();
            if (peek() == L']') break;
            if (peek() != L',') fail();
            position++;
        }

        position++;
        depth--;
        return node;
    }

    uint32_t hex() {
        if (position + 4 > text.size()) fail();

        uint32_t result = 0;
        for (int index = 0; index < 4; index++) {
            wchar_t character = text[position++];
            result <<= 4;

            if (character >= L'0' && character <= L'9') result |= static_cast<uint32_t>(character - L'0');
            else if (character >= L'a' && character <= L'f') result |= static_cast<uint32_t>(character - L'a' + 10);
            else if (character >= L'A' && character <= L'F') result |= static_cast<uint32_t>(character - L'A' + 10);
            else fail();
        }

        return result;
    }

    void unicode(std::wstring& result) {
        uint32_t unit = hex();

        if constexpr (sizeof(wchar_t) > 2) {
            bool isHigh = unit >= 0xD800 && unit < 0xDC00;

            if (isHigh && position + 1 < text.size() && text[position] == L'\\' && text[position + 1] == L'u') {
                size_t start = position;
                position += 2;
                uint32_t low = hex();

                if (low >= 0xDC00 && low < 0xE000) {
                    result += static_cast<wchar_t>(0x10000 + ((unit - 0xD800) << 10) + (low - 0xDC00));
                    return;
                }

                position = start;
            }
        }

        result += static_cast<wchar_t>(unit);
    }

    std::wstring string() {
        std::wstring result;
        position++;

        while (true) {
            if (position >= text.size()) fail();
            wchar_t character = text[position++];

            if (character == L'"') return result;
            if (static_cast<uint32_t>(character) < 0x20) fail();

            if (character != L'\\') {
                result += character;
                continue;
            }

            if (position >= text.size()) fail();

            switch (text[position++]) {
            case L'"':
                result += L'"';
                break;
            case L'\\':
                result += L'\\';
                break;
            case L'/':
                result += L'/';
                break;
            case L'b':
                result += L'\b';
                break;
            case L'f':
                result += L'\f';
                break;
            case L'n':
                result += L'\n';
                break;
            case L'r':
                result += L'\r';
                break;
            case L't':
                result += L'\t';
                break;
            case L'u':
                unicode(result);
                break;
            default:
                fail();
            }
        }
    }

    bool digits(std::string& out) {
        size_t start = out.size();
        while (peek() >= L'0' && peek() <= L'9') out += static_cast<char>(text[position++]);
        return out.size() > start;
    }

    Node number() {
        std::string ascii;

        if (peek() == L'-') {
            ascii += '-';
            position++;
        }

        if (peek() == L'0') {
            ascii += '0';
            position++;
        } else if (!digits(ascii)) {
            fail();
        }

        if (peek() == L'.') {
            ascii += '.';
            position++;
            if (!digits(ascii)) fail();
        }

        if (peek() == L'e' || peek() == L'E') {
            ascii += 'e';
            position++;

            if (peek() == L'+' || peek() == L'-') ascii += static_cast<char>(text[position++]);
            if (!digits(ascii)) fail();
        }

        auto node = makeNode(JsonValueType::Number);
        auto result = std::from_chars(ascii.data(), ascii.data() + ascii.size(), node->number);
        if (result.ec != std::errc()) fail();
        return node;
    }

    std::wstring_view text;
    size_t position = 0;
    int depth = 0;
};

Node parseRoot(std::wstring_view text, std::optional<JsonValueType> type) {
    auto node = Parser(text).parse();
    if (type && node->type != *type) throw JsonError("The JSON root has another type");
    return node;
}

}

JsonValueType IJsonValue::ValueType() const {
    return node->type;
}

std::wstring IJsonValue::GetString() const {
    return expect(node, JsonValueType::String).string;
}

double IJsonValue::GetNumber() const {
    return expect(node, JsonValueType::Number).number;
}

bool IJsonValue::GetBoolean() const {
    return expect(node, JsonValueType::Boolean).boolean;
}

JsonObject IJsonValue::GetObject() const {
    expect(node, JsonValueType::Object);
    return JsonObject(node);
}

JsonArray IJsonValue::GetArray() const {
    expect(node, JsonValueType::Array);
    return JsonArray(node);
}

std::wstring IJsonValue::Stringify() const {
    std::wstring out;
    write(out, node);
    return out;
}

JsonValue JsonValue::CreateNullValue() {
    return JsonValue(makeNode(JsonValueType::Null));
}

JsonValue JsonValue::CreateBooleanValue(bool value) {
    auto node = makeNode(JsonValueType::Boolean);
    node->boolean = value;
    return JsonValue(std::move(node));
}

JsonValue JsonValue::CreateNumberValue(double value) {
    auto node = makeNode(JsonValueType::Number);
    node->number = value;
    return JsonValue(std::move(node));
}

JsonValue JsonValue::CreateStringValue(std::wstring_view value) {
    auto node = makeNode(JsonValueType::String);
    node->string = value;
    return JsonValue(std::move(node));
}

JsonValue JsonValue::Parse(std::wstring_view text) {
    return JsonValue(parseRoot(text, std::nullopt));
}

bool JsonValue::TryParse(std::wstring_view text, JsonValue& result) {
    try {
        result = Parse(text);
        return true;
    } catch (JsonError const&) {
        return false;
    }
}

JsonObject::JsonObject() : IJsonValue(makeNode(JsonValueType::Object)) {}

JsonObject JsonObject::Parse(std::wstring_view text) {
    return JsonObject(parseRoot(text, JsonValueType::Object));
}

bool JsonObject::TryParse(std::wstring_view text, JsonObject& result) {
    try {
        result = Parse(text);
        return true;
    } catch (JsonError const&) {
        return false;
    }
}

std::shared_ptr<JsonNode> JsonObject::find(std::wstring_view key) const {
    std::lock_guard lock(node->mutex);

    for (auto const& [name, child] : node->members) {
        if (name == key) return child;
    }

    return nullptr;
}

bool JsonObject::HasKey(std::wstring_view key) const {
    return find(key) != nullptr;
}

uint32_t JsonObject::Size() const {
    std::lock_guard lock(node->mutex);
    return static_cast<uint32_t>(node->members.size());
}

JsonValue JsonObject::GetNamedValue(std::wstring_view key) const {
    auto child = find(key);
    if (!child) throw JsonError("The JSON object has no value named " + toUtf8(key));
    return JsonValue(std::move(child));
}

JsonValue JsonObject::GetNamedValue(std::wstring_view key, JsonValue const& fallback) const {
    auto child = find(key);
    return child ? JsonValue(std::move(child)) : fallback;
}

std::wstring JsonObject::GetNamedString(std::wstring_view key) const {
    return GetNamedValue(key).GetString();
}

std::wstring JsonObject::GetNamedString(std::wstring_view key, std::wstring_view fallback) const {
    return HasKey(key) ? GetNamedString(key) : std::wstring(fallback);
}

double JsonObject::GetNamedNumber(std::wstring_view key) const {
    return GetNamedValue(key).GetNumber();
}

double JsonObject::GetNamedNumber(std::wstring_view key, double fallback) const {
    return HasKey(key) ? GetNamedNumber(key) : fallback;
}

bool JsonObject::GetNamedBoolean(std::wstring_view key) const {
    return GetNamedValue(key).GetBoolean();
}

bool JsonObject::GetNamedBoolean(std::wstring_view key, bool fallback) const {
    return HasKey(key) ? GetNamedBoolean(key) : fallback;
}

JsonObject JsonObject::GetNamedObject(std::wstring_view key) const {
    return GetNamedValue(key).GetObject();
}

JsonObject JsonObject::GetNamedObject(std::wstring_view key, JsonObject const& fallback) const {
    return HasKey(key) ? GetNamedObject(key) : fallback;
}

JsonArray JsonObject::GetNamedArray(std::wstring_view key) const {
    return GetNamedValue(key).GetArray();
}

JsonArray JsonObject::GetNamedArray(std::wstring_view key, JsonArray const& fallback) const {
    return HasKey(key) ? GetNamedArray(key) : fallback;
}

void JsonObject::SetNamedValue(std::wstring_view key, IJsonValue const& value) {
    auto child = value ? value.node : makeNode(JsonValueType::Null);
    std::lock_guard lock(node->mutex);

    for (auto& [name, existing] : node->members) {
        if (name != key) continue;
        existing = std::move(child);
        return;
    }

    node->members.emplace_back(std::wstring(key), std::move(child));
}

void JsonObject::Remove(std::wstring_view key) {
    std::lock_guard lock(node->mutex);
    std::erase_if(node->members, [&](auto const& member) { return member.first == key; });
}

JsonIterator<JsonMember> JsonObject::begin() const {
    std::vector<JsonMember> members;
    std::lock_guard lock(node->mutex);

    for (auto const& [name, child] : node->members) members.emplace_back(name, JsonValue(child));
    return JsonIterator<JsonMember>(std::move(members));
}

JsonArray::JsonArray() : IJsonValue(makeNode(JsonValueType::Array)) {}

bool JsonArray::TryParse(std::wstring_view text, JsonArray& result) {
    try {
        result = JsonArray(parseRoot(text, JsonValueType::Array));
        return true;
    } catch (JsonError const&) {
        return false;
    }
}

uint32_t JsonArray::Size() const {
    std::lock_guard lock(node->mutex);
    return static_cast<uint32_t>(node->items.size());
}

IJsonValue JsonArray::GetAt(uint32_t index) const {
    std::lock_guard lock(node->mutex);
    if (index >= node->items.size()) throw JsonError("The JSON array index is out of range");
    return IJsonValue(node->items[index]);
}

std::wstring JsonArray::GetStringAt(uint32_t index) const {
    return GetAt(index).GetString();
}

double JsonArray::GetNumberAt(uint32_t index) const {
    return GetAt(index).GetNumber();
}

bool JsonArray::GetBooleanAt(uint32_t index) const {
    return GetAt(index).GetBoolean();
}

JsonObject JsonArray::GetObjectAt(uint32_t index) const {
    return GetAt(index).GetObject();
}

JsonArray JsonArray::GetArrayAt(uint32_t index) const {
    return GetAt(index).GetArray();
}

void JsonArray::Append(IJsonValue const& value) {
    auto child = value ? value.node : makeNode(JsonValueType::Null);
    std::lock_guard lock(node->mutex);
    node->items.push_back(std::move(child));
}

JsonIterator<IJsonValue> JsonArray::begin() const {
    std::vector<IJsonValue> items;
    std::lock_guard lock(node->mutex);

    for (auto const& child : node->items) items.push_back(IJsonValue(child));
    return JsonIterator<IJsonValue>(std::move(items));
}

}
