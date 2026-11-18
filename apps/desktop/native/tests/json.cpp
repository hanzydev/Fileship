#include "test.h"

#include <shared/json.h>

#include <optional>

using namespace fileship;

namespace {

template <typename Value>
std::wstring text(Value const& value) {
    return std::wstring(value.Stringify());
}

std::wstring escaped(std::wstring json) {
    for (auto& character : json) {
        if (character == L'%') character = L'\\';
    }

    return json;
}

std::wstring numberText(double number) {
    return text(JsonValue::CreateNumberValue(number));
}

std::wstring stringText(std::wstring const& value) {
    return text(JsonValue::CreateStringValue(value));
}

std::wstring roundTrip(std::wstring const& json) {
    return text(JsonValue::Parse(json));
}

bool parses(std::wstring const& json) {
    JsonValue value{nullptr};
    return JsonValue::TryParse(json, value);
}

}

TEST(jsonParsesEveryValueType) {
    auto parsed = JsonObject::Parse(LR"({"s":"a","n":1.5,"t":true,"f":false,"z":null,"o":{"k":1},"a":[1,"x"]})");

    CHECK(parsed.GetNamedValue(L"s").ValueType() == JsonValueType::String);
    CHECK(parsed.GetNamedValue(L"n").ValueType() == JsonValueType::Number);
    CHECK(parsed.GetNamedValue(L"t").ValueType() == JsonValueType::Boolean);
    CHECK(parsed.GetNamedValue(L"f").ValueType() == JsonValueType::Boolean);
    CHECK(parsed.GetNamedValue(L"z").ValueType() == JsonValueType::Null);
    CHECK(parsed.GetNamedValue(L"o").ValueType() == JsonValueType::Object);
    CHECK(parsed.GetNamedValue(L"a").ValueType() == JsonValueType::Array);
    CHECK(parsed.ValueType() == JsonValueType::Object);
    CHECK(parsed.Size() == 7);
}

TEST(jsonReadsNamedValues) {
    auto parsed = JsonObject::Parse(LR"({"s":"a","n":1.5,"t":true,"o":{"k":1},"a":[1,"x"]})");

    CHECK_EQUAL(parsed.GetNamedString(L"s"), L"a");
    CHECK(parsed.GetNamedNumber(L"n") == 1.5);
    CHECK(parsed.GetNamedBoolean(L"t"));
    CHECK(parsed.GetNamedObject(L"o").GetNamedNumber(L"k") == 1);
    CHECK(parsed.GetNamedArray(L"a").Size() == 2);
    CHECK_EQUAL(parsed.GetNamedValue(L"s").GetString(), L"a");
    CHECK(parsed.GetNamedValue(L"n").GetNumber() == 1.5);
    CHECK(parsed.GetNamedValue(L"t").GetBoolean());
    CHECK(parsed.GetNamedValue(L"o").GetObject().HasKey(L"k"));
    CHECK(parsed.GetNamedValue(L"a").GetArray().Size() == 2);
}

TEST(jsonThrowsOnMissingOrMismatchedValues) {
    auto parsed = JsonObject::Parse(LR"({"s":"a","n":1,"z":null})");

    CHECK_THROWS(parsed.GetNamedValue(L"missing"));
    CHECK_THROWS(parsed.GetNamedString(L"missing"));
    CHECK_THROWS(parsed.GetNamedString(L"n"));
    CHECK_THROWS(parsed.GetNamedNumber(L"s"));
    CHECK_THROWS(parsed.GetNamedBoolean(L"s"));
    CHECK_THROWS(parsed.GetNamedObject(L"s"));
    CHECK_THROWS(parsed.GetNamedArray(L"s"));
    CHECK_THROWS(parsed.GetNamedString(L"z"));
    CHECK_THROWS(parsed.GetNamedObject(L"z"));
    CHECK_THROWS(parsed.GetNamedArray(L"z"));
    CHECK_THROWS(parsed.GetNamedValue(L"s").GetNumber());
    CHECK_THROWS(parsed.GetNamedValue(L"n").GetString());
    CHECK_THROWS(parsed.GetNamedValue(L"n").GetObject());
    CHECK_THROWS(parsed.GetNamedValue(L"n").GetArray());
    CHECK_THROWS(parsed.GetNamedValue(L"z").GetBoolean());
}

TEST(jsonDefaultsApplyOnlyToMissingKeys) {
    auto parsed = JsonObject::Parse(LR"({"s":"a","n":2,"t":true,"z":null,"o":{"k":1}})");

    CHECK_EQUAL(parsed.GetNamedString(L"s", L"d"), L"a");
    CHECK_EQUAL(parsed.GetNamedString(L"missing", L"d"), L"d");
    CHECK(parsed.GetNamedNumber(L"n", 7) == 2);
    CHECK(parsed.GetNamedNumber(L"missing", 7) == 7);
    CHECK(parsed.GetNamedBoolean(L"t", false));
    CHECK(parsed.GetNamedBoolean(L"missing", true));
    CHECK_EQUAL(text(parsed.GetNamedObject(L"o", JsonObject())), LR"({"k":1})");
    CHECK_EQUAL(text(parsed.GetNamedObject(L"missing", JsonObject())), L"{}");
    CHECK_EQUAL(text(parsed.GetNamedArray(L"missing", JsonArray())), L"[]");
    CHECK_EQUAL(text(parsed.GetNamedValue(L"missing", JsonValue::CreateStringValue(L"d"))), LR"("d")");
    CHECK_EQUAL(text(parsed.GetNamedValue(L"z", JsonValue::CreateStringValue(L"d"))), L"null");
    CHECK_THROWS(parsed.GetNamedObject(L"s", JsonObject()));
    CHECK_THROWS(parsed.GetNamedArray(L"s", JsonArray()));
    CHECK_THROWS(parsed.GetNamedObject(L"z", JsonObject()));
    CHECK_THROWS(parsed.GetNamedString(L"n", L"d"));
    CHECK_THROWS(parsed.GetNamedNumber(L"s", 7));
    CHECK_THROWS(parsed.GetNamedBoolean(L"s", true));
    CHECK_THROWS(parsed.GetNamedString(L"z", L"d"));
    CHECK_THROWS(parsed.GetNamedNumber(L"z", 7));
    CHECK_THROWS(parsed.GetNamedBoolean(L"z", true));
}

TEST(jsonStringifiesNumbers) {
    CHECK_EQUAL(numberText(0), L"0");
    CHECK_EQUAL(numberText(1), L"1");
    CHECK_EQUAL(numberText(-2), L"-2");
    CHECK_EQUAL(numberText(1.5), L"1.5");
    CHECK_EQUAL(numberText(0.1), L"0.1");
    CHECK_EQUAL(numberText(-0.25), L"-0.25");
    CHECK_EQUAL(numberText(3.0), L"3");
    CHECK_EQUAL(numberText(1700000000000.0), L"1700000000000");
    CHECK_EQUAL(numberText(9007199254740991.0), L"9007199254740991");
    CHECK_EQUAL(numberText(123456.789), L"123456.789");
    CHECK_EQUAL(numberText(1e21), L"1E+21");
    CHECK_EQUAL(numberText(1.5e300), L"1.5E+300");
    CHECK_EQUAL(numberText(1e-7), L"1E-7");
    CHECK_EQUAL(numberText(0.000123), L"0.000123");
    CHECK_EQUAL(numberText(1.0 / 3.0), L"0.33333333333333331");
    CHECK_EQUAL(numberText(1e14), L"100000000000000");
    CHECK_EQUAL(numberText(1e15), L"1E+15");
    CHECK_EQUAL(numberText(1.2e15), L"1.2E+15");
    CHECK_EQUAL(numberText(1234567890123456.0), L"1234567890123456");
    CHECK_EQUAL(numberText(123456789012345678.0), L"1.2345678901234568E+17");
    CHECK_EQUAL(numberText(0.0001), L"0.0001");
    CHECK_EQUAL(numberText(0.00001), L"1E-5");
    CHECK_EQUAL(numberText(0.000015), L"1.5E-5");
    CHECK_EQUAL(numberText(2.5e-10), L"2.5E-10");
    CHECK_EQUAL(numberText(-1e21), L"-1E+21");
    CHECK_EQUAL(numberText(4.35), L"4.35");
    CHECK_EQUAL(numberText(100.0 / 3.0), L"33.333333333333336");
    CHECK_EQUAL(numberText(0.1 + 0.2), L"0.30000000000000004");
    CHECK_EQUAL(numberText(1.7976931348623157e308), L"1.7976931348623157E+308");
    CHECK_EQUAL(numberText(5e-324), L"4.94065645841247E-324");
    CHECK_EQUAL(numberText(-0.0), L"-0");
}

TEST(jsonStringifiesStrings) {
    CHECK_EQUAL(stringText(L""), LR"("")");
    CHECK_EQUAL(stringText(L"plain"), LR"("plain")");
    CHECK_EQUAL(stringText(L"quote\"back\\slash"), LR"("quote\"back\\slash")");
    CHECK_EQUAL(stringText(L"line\nfeed\ttab\rreturn"), LR"("line\nfeed\ttab\rreturn")");
    CHECK_EQUAL(stringText(L"\b\f"), LR"("\b\f")");
    CHECK_EQUAL(stringText(L"\x01"), escaped(LR"("%u0001")"));
    CHECK_EQUAL(stringText(L"\x0b\x1f"), escaped(LR"("%u000B%u001F")"));
    CHECK_EQUAL(stringText(L"\x7f"), L"\"\x7f\"");
    CHECK_EQUAL(stringText(L"a/b"), LR"("a/b")");
    CHECK_EQUAL(stringText(L"\xe9\x15f"), L"\"\xe9\x15f\"");
    CHECK_EQUAL(stringText(L"\U0001F600"), L"\"\U0001F600\"");
    CHECK_EQUAL(stringText(L"<>&'"), LR"("<>&'")");
}

TEST(jsonStringifiesContainers) {
    CHECK_EQUAL(text(JsonObject()), L"{}");
    CHECK_EQUAL(text(JsonArray()), L"[]");
    CHECK_EQUAL(text(JsonValue::CreateNullValue()), L"null");
    CHECK_EQUAL(text(JsonValue::CreateBooleanValue(true)), L"true");
    CHECK_EQUAL(text(JsonValue::CreateBooleanValue(false)), L"false");
    CHECK_EQUAL(roundTrip(LR"( { "b" : [ 1 , { "c" : null } ] , "a" : "x" } )"), LR"({"b":[1,{"c":null}],"a":"x"})");
}

TEST(jsonStringifiesKeysInInsertionOrder) {
    JsonObject object;
    object.SetNamedValue(L"zebra", JsonValue::CreateNumberValue(1));
    object.SetNamedValue(L"apple", JsonValue::CreateNumberValue(2));
    object.SetNamedValue(L"mango", JsonValue::CreateNumberValue(3));
    object.SetNamedValue(L"zebra", JsonValue::CreateNumberValue(4));
    CHECK_EQUAL(text(object), LR"({"zebra":4,"apple":2,"mango":3})");

    object.Remove(L"zebra");
    object.SetNamedValue(L"zebra", JsonValue::CreateNumberValue(5));
    CHECK_EQUAL(text(object), LR"({"apple":2,"mango":3,"zebra":5})");

    CHECK_EQUAL(roundTrip(LR"({"b":1,"a":2,"c":3})"), LR"({"b":1,"a":2,"c":3})");
}

TEST(jsonIteratesKeysInInsertionOrder) {
    auto object = JsonObject::Parse(LR"({"zebra":4,"apple":2,"mango":3})");

    std::wstring keys;
    for (auto const& pair : object) keys += std::wstring(pair.Key()) + L"=" + text(pair.Value()) + L";";
    CHECK_EQUAL(keys, L"zebra=4;apple=2;mango=3;");
}

TEST(jsonParsesEscapesAndNumbers) {
    CHECK_EQUAL(JsonValue::Parse(LR"("a\"b\\c\/d\n\t\r\b\f")").GetString(), L"a\"b\\c/d\n\t\r\b\f");
    CHECK_EQUAL(JsonValue::Parse(escaped(LR"("%u00e9%u015F")")).GetString(), L"\xe9\x15f");
    CHECK_EQUAL(JsonValue::Parse(escaped(LR"("%ud83d%ude00")")).GetString(), L"\U0001F600");
    CHECK_EQUAL(JsonValue::Parse(L"\"\xe9\U0001F600\"").GetString(), L"\xe9\U0001F600");
    CHECK(JsonValue::Parse(L"1e3").GetNumber() == 1000);
    CHECK(JsonValue::Parse(L"-0.5").GetNumber() == -0.5);
    CHECK(JsonValue::Parse(L"12345678901234567890").GetNumber() == 12345678901234567890.0);
    CHECK(JsonValue::Parse(L"1.5E-3").GetNumber() == 0.0015);
    CHECK(JsonValue::Parse(L"5e-324").GetNumber() == 5e-324);
    CHECK(JsonValue::Parse(L" \t\r\n true \n").GetBoolean());
    CHECK(JsonValue::Parse(L"null").ValueType() == JsonValueType::Null);
    CHECK_EQUAL(JsonValue::Parse(LR"("top")").GetString(), L"top");
    CHECK_EQUAL(roundTrip(L"-0"), L"-0");
    CHECK_EQUAL(roundTrip(L"1.0"), L"1");
    CHECK_EQUAL(roundTrip(L"1.50"), L"1.5");
    CHECK_EQUAL(roundTrip(escaped(LR"("%u0000")")), escaped(LR"("%u0000")"));
    CHECK_EQUAL(roundTrip(LR"({"":1})"), LR"({"":1})");
    CHECK_EQUAL(roundTrip(L"\"\x7f\""), L"\"\x7f\"");
}

TEST(jsonRejectsInvalidText) {
    CHECK(!parses(L""));
    CHECK(!parses(L"   "));
    CHECK(!parses(L"{"));
    CHECK(!parses(LR"({"a":1,})"));
    CHECK(!parses(L"[1,]"));
    CHECK(!parses(LR"({'a':1})"));
    CHECK(!parses(LR"({a:1})"));
    CHECK(!parses(L"nul"));
    CHECK(!parses(L"True"));
    CHECK(!parses(L"01"));
    CHECK(!parses(L"1."));
    CHECK(!parses(L".5"));
    CHECK(!parses(L"+1"));
    CHECK(!parses(L"1 2"));
    CHECK(!parses(LR"({"a":1} x)"));
    CHECK(!parses(LR"("unterminated)"));
    CHECK(!parses(L"\"line\nbreak\""));
    CHECK(!parses(L"\"tab\tinside\""));
    CHECK(!parses(LR"("\x41")"));
    CHECK(!parses(escaped(LR"("%u12")")));
    CHECK(!parses(L"// comment\n1"));
    CHECK(!parses(L"NaN"));
    CHECK(!parses(L"\xfeff{}"));
    CHECK(!parses(L"{}\xa0"));
}

TEST(jsonRejectsNumbersOutOfRange) {
    CHECK(!parses(L"1E400"));
    CHECK(!parses(L"1e-400"));
    CHECK(!parses(L"[-1E400]"));
}

TEST(jsonTypedParsersCheckTheRoot) {
    JsonObject object{nullptr};
    JsonArray array{nullptr};

    CHECK(JsonObject::TryParse(LR"({"a":1})", object));
    CHECK(object.GetNamedNumber(L"a") == 1);
    CHECK(!JsonObject::TryParse(L"[1]", object));
    CHECK(!JsonObject::TryParse(L"5", object));
    CHECK(JsonArray::TryParse(L"[1,2]", array));
    CHECK(array.Size() == 2);
    CHECK(!JsonArray::TryParse(LR"({"a":1})", array));
    CHECK_THROWS(JsonObject::Parse(L"[1]"));
    CHECK_THROWS(JsonObject::Parse(L"not json"));
    CHECK_THROWS(JsonValue::Parse(L"{"));
}

TEST(jsonDuplicateKeysKeepTheLastValue) {
    CHECK_EQUAL(roundTrip(LR"({"a":1,"b":2,"a":3})"), LR"({"a":3,"b":2})");
}

TEST(jsonHandlesShareOneValue) {
    JsonObject parent;
    JsonObject child;
    parent.SetNamedValue(L"child", child);

    child.SetNamedValue(L"late", JsonValue::CreateBooleanValue(true));
    CHECK_EQUAL(text(parent), LR"({"child":{"late":true}})");

    auto fetched = parent.GetNamedObject(L"child");
    fetched.SetNamedValue(L"more", JsonValue::CreateNumberValue(1));
    CHECK_EQUAL(text(parent), LR"({"child":{"late":true,"more":1}})");

    auto viaValue = parent.GetNamedValue(L"child").GetObject();
    viaValue.SetNamedValue(L"late", JsonValue::CreateBooleanValue(false));
    CHECK_EQUAL(text(child), LR"({"late":false,"more":1})");

    JsonObject copy = parent;
    copy.SetNamedValue(L"x", JsonValue::CreateNullValue());
    CHECK(parent.HasKey(L"x"));

    JsonArray list;
    parent.SetNamedValue(L"list", list);
    list.Append(JsonValue::CreateNumberValue(5));
    CHECK(parent.GetNamedArray(L"list").Size() == 1);

    JsonObject twice;
    twice.SetNamedValue(L"a", list);
    twice.SetNamedValue(L"b", list);
    list.Append(JsonValue::CreateNumberValue(6));
    CHECK_EQUAL(text(twice), LR"({"a":[5,6],"b":[5,6]})");
}

TEST(jsonParsedCopiesAreIndependent) {
    JsonObject original;
    original.SetNamedValue(L"a", JsonValue::CreateNumberValue(1));

    auto clone = JsonObject::Parse(original.Stringify());
    clone.SetNamedValue(L"a", JsonValue::CreateNumberValue(2));

    CHECK(original.GetNamedNumber(L"a") == 1);
}

TEST(jsonObjectsEdit) {
    JsonObject object;
    CHECK(object.Size() == 0);
    CHECK(!object.HasKey(L"a"));

    object.SetNamedValue(L"a", JsonValue::CreateStringValue(L"x"));
    object.SetNamedValue(L"b", JsonValue::CreateNumberValue(2));
    CHECK(object.HasKey(L"a"));
    CHECK(object.Size() == 2);

    object.Remove(L"a");
    object.Remove(L"missing");
    CHECK(!object.HasKey(L"a"));
    CHECK(object.Size() == 1);
    CHECK_EQUAL(text(object), LR"({"b":2})");
}

TEST(jsonArraysEdit) {
    JsonArray array;
    array.Append(JsonValue::CreateStringValue(L"s"));
    array.Append(JsonValue::CreateNumberValue(4));
    array.Append(JsonValue::CreateBooleanValue(true));
    array.Append(JsonObject::Parse(LR"({"k":1})"));
    array.Append(JsonArray());
    array.Append(JsonValue::CreateNullValue());

    CHECK(array.Size() == 6);
    CHECK_EQUAL(array.GetStringAt(0), L"s");
    CHECK(array.GetNumberAt(1) == 4);
    CHECK(array.GetBooleanAt(2));
    CHECK(array.GetObjectAt(3).GetNamedNumber(L"k") == 1);
    CHECK(array.GetArrayAt(4).Size() == 0);
    CHECK(array.GetAt(5).ValueType() == JsonValueType::Null);
    CHECK(array.GetAt(0).ValueType() == JsonValueType::String);
    CHECK_EQUAL(array.GetAt(0).GetString(), L"s");
    CHECK_THROWS(array.GetStringAt(1));
    CHECK_THROWS(array.GetObjectAt(0));
    CHECK_THROWS(array.GetAt(6));
    CHECK_EQUAL(text(array), LR"(["s",4,true,{"k":1},[],null])");

    std::wstring types;
    for (auto const& item : array) types += item.ValueType() == JsonValueType::String ? L"s" : L"-";
    CHECK_EQUAL(types, L"s-----");
}

TEST(jsonNullHandlesAreFalse) {
    JsonObject object{nullptr};
    JsonArray array{nullptr};
    JsonValue value{nullptr};

    CHECK(!object);
    CHECK(!array);
    CHECK(!value);
    CHECK(!!JsonObject());
    CHECK(!!JsonArray());
    CHECK(!!JsonValue::CreateNullValue());

    std::optional<IJsonValue> body;
    CHECK(!body);
    body = JsonObject();
    CHECK(body->ValueType() == JsonValueType::Object);

    IJsonValue generic = JsonValue::CreateStringValue(L"x");
    CHECK(!!generic);
    CHECK_EQUAL(generic.GetString(), L"x");
    generic = JsonArray();
    CHECK(generic.ValueType() == JsonValueType::Array);
}

TEST(jsonStringsCompareLikeText) {
    auto value = JsonValue::CreateStringValue(L"Admin");
    std::wstring same = L"Admin";

    CHECK(value.GetString() == L"Admin");
    CHECK(value.GetString() == same);
    CHECK(value.GetString().size() == 5);
    CHECK(!(value.GetString() == L"admin"));
}

TEST(jsonValuesCompareByText) {
    auto first = JsonObject::Parse(LR"({"enum":["a",1,null]})").GetNamedArray(L"enum");

    CHECK(first.GetAt(0).Stringify() == JsonValue::CreateStringValue(L"a").Stringify());
    CHECK(first.GetAt(1).Stringify() == JsonValue::CreateNumberValue(1).Stringify());
    CHECK(first.GetAt(2).Stringify() == JsonValue::CreateNullValue().Stringify());
}
