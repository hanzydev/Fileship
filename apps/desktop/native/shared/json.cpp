#include "json.h"

#include <charconv>
#include <cmath>

namespace fileship {

struct JsonNode {
    JsonValueType type = JsonValueType::Null;
    bool boolean = false;
    double number = 0;
    std::wstring string;
    JsonObject::Members members;
    JsonArray::Items items;
};

struct JsonAccess {
    static JsonNode const& node(IJsonValue const& value) {
        return *value.node;
    }
};

namespace {

std::shared_ptr<JsonNode> makeNode(JsonValueType type) {
    auto node = std::make_shared<JsonNode>();
    node->type = type;
    return node;
}

void appendCodePoint(std::string& out, uint32_t code) {
    if (code < 0x80) {
        out += static_cast<char>(code);
    } else if (code < 0x800) {
        out += static_cast<char>(0xC0 | (code >> 6));
        out += static_cast<char>(0x80 | (code & 0x3F));
    } else if (code < 0x10000) {
        out += static_cast<char>(0xE0 | (code >> 12));
        out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (code & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (code >> 18));
        out += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (code & 0x3F));
    }
}

class Parser {
public:
    explicit Parser(std::string_view text) : text(text) {}

    IJsonValue document() {
        auto value = parseValue(0);
        skipSpace();
        if (position != text.size()) fail("Unexpected trailing characters");
        return value;
    }

private:
    [[noreturn]] void fail(char const* message) const {
        throw JsonError(std::string(message) + " at position " + std::to_string(position));
    }

    void skipSpace() {
        while (position < text.size()) {
            char c = text[position];
            if (c != ' ' && c != '\t' && c != '\n' && c != '\r') break;
            position++;
        }
    }

    bool consume(std::string_view word) {
        if (text.substr(position, word.size()) != word) return false;
        position += word.size();
        return true;
    }

    IJsonValue parseValue(int depth) {
        if (depth > 512) fail("Nesting is too deep");
        skipSpace();
        if (position >= text.size()) fail("Unexpected end of input");

        char c = text[position];
        if (c == '{') return parseObject(depth);
        if (c == '[') return parseArray(depth);
        if (c == '"') return JsonValue::CreateStringValue(toWide(parseString()));
        if (consume("true")) return JsonValue::CreateBooleanValue(true);
        if (consume("false")) return JsonValue::CreateBooleanValue(false);
        if (consume("null")) return JsonValue::CreateNullValue();
        if (c == '-' || (c >= '0' && c <= '9')) return JsonValue::CreateNumberValue(parseNumber());
        fail("Unexpected character");
    }

    IJsonValue parseObject(int depth) {
        position++;
        JsonObject object;
        skipSpace();
        if (position < text.size() && text[position] == '}') {
            position++;
            return object;
        }

        while (true) {
            skipSpace();
            if (position >= text.size() || text[position] != '"') fail("Expected a key");
            auto key = toWide(parseString());
            skipSpace();
            if (position >= text.size() || text[position] != ':') fail("Expected ':'");
            position++;
            object.SetNamedValue(key, parseValue(depth + 1));
            skipSpace();
            if (position >= text.size()) fail("Unexpected end of input");
            if (text[position] == ',') {
                position++;
                continue;
            }
            if (text[position] != '}') fail("Expected ',' or '}'");
            position++;
            return object;
        }
    }

    IJsonValue parseArray(int depth) {
        position++;
        JsonArray array;
        skipSpace();
        if (position < text.size() && text[position] == ']') {
            position++;
            return array;
        }

        while (true) {
            array.Append(parseValue(depth + 1));
            skipSpace();
            if (position >= text.size()) fail("Unexpected end of input");
            if (text[position] == ',') {
                position++;
                continue;
            }
            if (text[position] != ']') fail("Expected ',' or ']'");
            position++;
            return array;
        }
    }

    uint32_t parseHex4() {
        if (position + 4 > text.size()) fail("Invalid escape");
        uint32_t value = 0;
        for (int index = 0; index < 4; index++) {
            char c = text[position++];
            value <<= 4;
            if (c >= '0' && c <= '9') value |= c - '0';
            else if (c >= 'a' && c <= 'f') value |= c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') value |= c - 'A' + 10;
            else fail("Invalid escape");
        }
        return value;
    }

    std::string parseString() {
        position++;
        std::string out;

        while (true) {
            if (position >= text.size()) fail("Unterminated string");
            char c = text[position++];
            if (c == '"') return out;
            if (static_cast<unsigned char>(c) < 0x20) fail("Control character in string");
            if (c != '\\') {
                out += c;
                continue;
            }

            if (position >= text.size()) fail("Invalid escape");
            switch (text[position++]) {
            case '"':
                out += '"';
                break;
            case '\\':
                out += '\\';
                break;
            case '/':
                out += '/';
                break;
            case 'b':
                out += '\b';
                break;
            case 'f':
                out += '\f';
                break;
            case 'n':
                out += '\n';
                break;
            case 'r':
                out += '\r';
                break;
            case 't':
                out += '\t';
                break;
            case 'u': {
                uint32_t code = parseHex4();
                if (code >= 0xD800 && code < 0xDC00 && text.substr(position, 2) == "\\u") {
                    auto saved = position;
                    position += 2;
                    uint32_t low = parseHex4();
                    if (low >= 0xDC00 && low < 0xE000) code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                    else position = saved;
                }
                if (code >= 0xD800 && code < 0xE000) code = 0xFFFD;
                appendCodePoint(out, code);
                break;
            }
            default:
                fail("Invalid escape");
            }
        }
    }

    double parseNumber() {
        auto start = position;
        if (text[position] == '-') position++;
        auto digits = [&] {
            auto from = position;
            while (position < text.size() && text[position] >= '0' && text[position] <= '9') position++;
            return position > from;
        };
        if (!digits()) fail("Invalid number");
        if (position < text.size() && text[position] == '.') {
            position++;
            if (!digits()) fail("Invalid number");
        }
        if (position < text.size() && (text[position] == 'e' || text[position] == 'E')) {
            position++;
            if (position < text.size() && (text[position] == '+' || text[position] == '-')) position++;
            if (!digits()) fail("Invalid number");
        }

        double value = 0;
        auto result = std::from_chars(text.data() + start, text.data() + position, value);
        if (result.ec == std::errc::result_out_of_range) return text[start] == '-' ? -HUGE_VAL : HUGE_VAL;
        if (result.ec != std::errc()) fail("Invalid number");
        return value;
    }

    std::string_view text;
    size_t position = 0;
};

void writeString(std::string& out, std::wstring_view value) {
    static constexpr char HEX[] = "0123456789abcdef";
    out += '"';
    for (char c : toUtf8(value)) {
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\b':
            out += "\\b";
            break;
        case '\f':
            out += "\\f";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                out += "\\u00";
                out += HEX[(c >> 4) & 0xF];
                out += HEX[c & 0xF];
            } else {
                out += c;
            }
        }
    }
    out += '"';
}

void writeNumber(std::string& out, double value) {
    if (!std::isfinite(value)) {
        out += "null";
        return;
    }

    char buffer[32];
    auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    out.append(buffer, result.ptr);
}

void write(std::string& out, JsonNode const& node) {
    switch (node.type) {
    case JsonValueType::Null:
        out += "null";
        return;
    case JsonValueType::Boolean:
        out += node.boolean ? "true" : "false";
        return;
    case JsonValueType::Number:
        writeNumber(out, node.number);
        return;
    case JsonValueType::String:
        writeString(out, node.string);
        return;
    case JsonValueType::Array: {
        out += '[';
        bool first = true;
        for (auto const& item : node.items) {
            if (!first) out += ',';
            first = false;
            write(out, JsonAccess::node(item));
        }
        out += ']';
        return;
    }
    case JsonValueType::Object: {
        out += '{';
        bool first = true;
        for (auto const& [key, value] : node.members) {
            if (!first) out += ',';
            first = false;
            writeString(out, key);
            out += ':';
            write(out, JsonAccess::node(value));
        }
        out += '}';
        return;
    }
    }
}

char const* typeName(JsonValueType type) {
    switch (type) {
    case JsonValueType::Null:
        return "null";
    case JsonValueType::Boolean:
        return "a boolean";
    case JsonValueType::Number:
        return "a number";
    case JsonValueType::String:
        return "a string";
    case JsonValueType::Array:
        return "an array";
    case JsonValueType::Object:
        return "an object";
    }
    return "unknown";
}

}

JsonNode& IJsonValue::checked() const {
    if (!node) throw JsonError("The JSON value is null");
    return *node;
}

JsonNode& IJsonValue::checked(JsonValueType type) const {
    auto& value = checked();
    if (value.type != type)
        throw JsonError(std::string("Expected ") + typeName(type) + ", got " + typeName(value.type));
    return value;
}

JsonValueType IJsonValue::ValueType() const {
    return checked().type;
}

std::wstring IJsonValue::GetString() const {
    return checked(JsonValueType::String).string;
}

double IJsonValue::GetNumber() const {
    return checked(JsonValueType::Number).number;
}

bool IJsonValue::GetBoolean() const {
    return checked(JsonValueType::Boolean).boolean;
}

JsonObject IJsonValue::GetObject() const {
    checked(JsonValueType::Object);
    return JsonObject(node);
}

JsonArray IJsonValue::GetArray() const {
    checked(JsonValueType::Array);
    return JsonArray(node);
}

std::wstring IJsonValue::Stringify() const {
    return toWide(stringify(*this));
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
    return JsonValue(parseJson(toUtf8(text)).node);
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
    return parseJson(toUtf8(text)).GetObject();
}

bool JsonObject::TryParse(std::wstring_view text, JsonObject& result) {
    auto parsed = parseObject(toUtf8(text));
    if (!parsed) return false;
    result = *parsed;
    return true;
}

uint32_t JsonObject::Size() const {
    return static_cast<uint32_t>(checked(JsonValueType::Object).members.size());
}

IJsonValue const* JsonObject::find(std::wstring_view key) const {
    for (auto const& member : checked(JsonValueType::Object).members)
        if (member.first == key) return &member.second;
    return nullptr;
}

IJsonValue const& JsonObject::lookup(std::wstring_view key) const {
    auto value = find(key);
    if (!value) throw JsonError("The JSON object has no key \"" + toUtf8(key) + "\"");
    return *value;
}

bool JsonObject::HasKey(std::wstring_view key) const {
    return find(key) != nullptr;
}

void JsonObject::SetNamedValue(std::wstring_view key, IJsonValue const& value) {
    if (!value) throw JsonError("Cannot store a null JSON value");

    auto& members = checked(JsonValueType::Object).members;
    for (auto& member : members) {
        if (member.first == key) {
            member.second = value;
            return;
        }
    }
    members.emplace_back(std::wstring(key), value);
}

bool JsonObject::Remove(std::wstring_view key) {
    auto& members = checked(JsonValueType::Object).members;
    return std::erase_if(members, [&](auto const& member) { return member.first == key; }) > 0;
}

JsonValue JsonObject::GetNamedValue(std::wstring_view key) const {
    return JsonValue(lookup(key).node);
}

JsonValue JsonObject::GetNamedValue(std::wstring_view key, JsonValue const& fallback) const {
    auto value = find(key);
    return value ? JsonValue(value->node) : fallback;
}

std::wstring JsonObject::GetNamedString(std::wstring_view key) const {
    return lookup(key).GetString();
}

std::wstring JsonObject::GetNamedString(std::wstring_view key, std::wstring_view fallback) const {
    auto value = find(key);
    return value ? value->GetString() : std::wstring(fallback);
}

double JsonObject::GetNamedNumber(std::wstring_view key) const {
    return lookup(key).GetNumber();
}

double JsonObject::GetNamedNumber(std::wstring_view key, double fallback) const {
    auto value = find(key);
    return value ? value->GetNumber() : fallback;
}

bool JsonObject::GetNamedBoolean(std::wstring_view key) const {
    return lookup(key).GetBoolean();
}

bool JsonObject::GetNamedBoolean(std::wstring_view key, bool fallback) const {
    auto value = find(key);
    return value ? value->GetBoolean() : fallback;
}

JsonObject JsonObject::GetNamedObject(std::wstring_view key) const {
    return lookup(key).GetObject();
}

JsonObject JsonObject::GetNamedObject(std::wstring_view key, JsonObject const& fallback) const {
    auto value = find(key);
    return value ? value->GetObject() : fallback;
}

JsonArray JsonObject::GetNamedArray(std::wstring_view key) const {
    return lookup(key).GetArray();
}

JsonArray JsonObject::GetNamedArray(std::wstring_view key, JsonArray const& fallback) const {
    auto value = find(key);
    return value ? value->GetArray() : fallback;
}

JsonObject::Iterator JsonObject::begin() const {
    return Iterator(checked(JsonValueType::Object).members.cbegin());
}

JsonObject::Iterator JsonObject::end() const {
    return Iterator(checked(JsonValueType::Object).members.cend());
}

JsonArray::JsonArray() : IJsonValue(makeNode(JsonValueType::Array)) {}

JsonArray JsonArray::Parse(std::wstring_view text) {
    return parseJson(toUtf8(text)).GetArray();
}

bool JsonArray::TryParse(std::wstring_view text, JsonArray& result) {
    auto parsed = parseArray(toUtf8(text));
    if (!parsed) return false;
    result = *parsed;
    return true;
}

uint32_t JsonArray::Size() const {
    return static_cast<uint32_t>(checked(JsonValueType::Array).items.size());
}

void JsonArray::Append(IJsonValue const& value) {
    if (!value) throw JsonError("Cannot store a null JSON value");
    checked(JsonValueType::Array).items.push_back(value);
}

IJsonValue JsonArray::GetAt(uint32_t index) const {
    auto const& items = checked(JsonValueType::Array).items;
    if (index >= items.size()) throw JsonError("JSON array index out of range");
    return items[index];
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

JsonArray::Items::const_iterator JsonArray::begin() const {
    return checked(JsonValueType::Array).items.cbegin();
}

JsonArray::Items::const_iterator JsonArray::end() const {
    return checked(JsonValueType::Array).items.cend();
}

IJsonValue parseJson(std::string_view text) {
    if (text.size() >= 3 && text.substr(0, 3) == "\xEF\xBB\xBF") text.remove_prefix(3);
    return Parser(text).document();
}

std::string stringify(IJsonValue const& value) {
    if (!value) throw JsonError("The JSON value is null");
    std::string out;
    write(out, JsonAccess::node(value));
    return out;
}

}
