#include "app.h"
#include "rpc.h"

#include <shared/log.h>

#include <bcrypt.h>
#include <dpapi.h>

#include <cmath>
#include <mutex>

namespace fileship::host {

namespace {

constexpr wchar_t const* PERMISSIONS[] = {L"Admin", L"UploadFiles", L"TakeNotes", L"ShortenLinks"};

std::recursive_mutex mutex;
JsonObject data{nullptr};
JsonObject schema{nullptr};
JsonObject defaults{nullptr};

std::filesystem::path storePath() {
    return userDataPath() / L"settings.json";
}

JsonObject readJsonFile(std::filesystem::path const& path) {
    auto bytes = readFileBytes(path);
    auto object = parseObject(std::string_view(bytes.data(), bytes.size()));
    return object ? *object : JsonObject();
}

struct InvalidSetting : std::runtime_error {
    using std::runtime_error::runtime_error;
};

bool matchesType(IJsonValue const& value, IJsonValue const& type) {
    auto check = [&](std::wstring const& name) {
        switch (value.ValueType()) {
        case JsonValueType::Null:
            return name == L"null";
        case JsonValueType::Boolean:
            return name == L"boolean";
        case JsonValueType::String:
            return name == L"string";
        case JsonValueType::Object:
            return name == L"object";
        case JsonValueType::Array:
            return name == L"array";
        case JsonValueType::Number:
            return name == L"number" || (name == L"integer" && std::floor(value.GetNumber()) == value.GetNumber());
        }

        return false;
    };

    if (type.ValueType() == JsonValueType::String) return check(std::wstring(type.GetString()));
    if (type.ValueType() != JsonValueType::Array) return true;

    for (auto const& item : type.GetArray()) {
        if (check(std::wstring(item.GetString()))) return true;
    }

    return false;
}

bool inEnum(JsonObject const& rule, IJsonValue const& value) {
    if (!rule.HasKey(L"enum")) return true;

    for (auto const& item : rule.GetNamedArray(L"enum")) {
        if (item.Stringify() == value.Stringify()) return true;
    }

    return false;
}

IJsonValue coerce(JsonObject const& rule, IJsonValue const& value, IJsonValue const& fallback, bool strict,
                  std::wstring const& path);

IJsonValue fail(IJsonValue const& fallback, bool strict, std::wstring const& path) {
    if (strict) throw InvalidSetting(toUtf8(L"Invalid setting: " + path));
    return fallback;
}

IJsonValue coerceObject(JsonObject const& rule, IJsonValue const& value, IJsonValue const& fallback, bool strict,
                        std::wstring const& path) {
    if (value.ValueType() != JsonValueType::Object) return fail(fallback, strict, path);

    auto input = value.GetObject();
    auto base = fallback.ValueType() == JsonValueType::Object ? fallback.GetObject() : JsonObject();
    JsonObject result;

    if (rule.HasKey(L"properties")) {
        for (auto const& pair : rule.GetNamedObject(L"properties")) {
            std::wstring key(pair.Key());
            IJsonValue childFallback = base.HasKey(key) ? base.GetNamedValue(key) : JsonValue::CreateNullValue();

            result.SetNamedValue(key, input.HasKey(key) ? coerce(pair.Value().GetObject(), input.GetNamedValue(key),
                                                                 childFallback, strict, path + L"." + key)
                                                        : childFallback);
        }

        return result;
    }

    for (auto const& pair : base) result.SetNamedValue(pair.Key(), pair.Value());

    auto names = rule.HasKey(L"propertyNames") ? rule.GetNamedObject(L"propertyNames") : JsonObject();
    auto items = rule.HasKey(L"additionalProperties") &&
                         rule.GetNamedValue(L"additionalProperties").ValueType() == JsonValueType::Object
                     ? rule.GetNamedObject(L"additionalProperties")
                     : JsonObject();

    for (auto const& pair : input) {
        std::wstring key(pair.Key());

        if (!inEnum(names, JsonValue::CreateStringValue(key))) {
            fail(fallback, strict, path + L"." + key);
            continue;
        }

        IJsonValue childFallback = base.HasKey(key) ? base.GetNamedValue(key) : JsonValue::CreateNullValue();
        auto child = coerce(items, pair.Value(), childFallback, strict, path + L"." + key);
        if (child.ValueType() != JsonValueType::Null || pair.Value().ValueType() == JsonValueType::Null)
            result.SetNamedValue(key, child);
    }

    return result;
}

IJsonValue coerce(JsonObject const& rule, IJsonValue const& value, IJsonValue const& fallback, bool strict,
                  std::wstring const& path) {
    if (rule.HasKey(L"type") && !matchesType(value, rule.GetNamedValue(L"type"))) return fail(fallback, strict, path);
    if (!inEnum(rule, value)) return fail(fallback, strict, path);

    if (value.ValueType() == JsonValueType::Object) return coerceObject(rule, value, fallback, strict, path);

    if (value.ValueType() == JsonValueType::Number) {
        double number = value.GetNumber();
        if ((rule.HasKey(L"minimum") && number < rule.GetNamedNumber(L"minimum")) ||
            (rule.HasKey(L"maximum") && number > rule.GetNamedNumber(L"maximum")))
            return fail(fallback, strict, path);
    }

    if (value.ValueType() == JsonValueType::String) {
        auto length = static_cast<double>(value.GetString().size());
        if ((rule.HasKey(L"minLength") && length < rule.GetNamedNumber(L"minLength")) ||
            (rule.HasKey(L"maxLength") && length > rule.GetNamedNumber(L"maxLength")))
            return fail(fallback, strict, path);
    }

    return value;
}

JsonObject normalizeSettings(IJsonValue const& input, bool strict) {
    if (!schema) {
        schema = readJsonFile(resourcesPath() / L"settings-schema.json");
        defaults = readJsonFile(resourcesPath() / L"settings-defaults.json");
    }

    IJsonValue value = input.ValueType() == JsonValueType::Object ? input : IJsonValue(JsonObject());
    return coerce(schema, value, defaults, strict, L"settings").GetObject();
}

std::optional<JsonObject> normalizeUser(IJsonValue const& value) {
    if (value.ValueType() != JsonValueType::Object) return std::nullopt;

    auto user = value.GetObject();
    if (!user.HasKey(L"id") || !user.HasKey(L"username") ||
        user.GetNamedValue(L"id").ValueType() != JsonValueType::String ||
        user.GetNamedValue(L"username").ValueType() != JsonValueType::String)
        return std::nullopt;

    JsonObject result;
    result.SetNamedValue(L"id", user.GetNamedValue(L"id"));
    result.SetNamedValue(L"username", user.GetNamedValue(L"username"));
    result.SetNamedValue(L"avatar", stringOrNull(jsonString(user, L"avatar")));

    JsonArray permissions;
    if (user.HasKey(L"permissions") && user.GetNamedValue(L"permissions").ValueType() == JsonValueType::Array) {
        for (auto const& item : user.GetNamedArray(L"permissions")) {
            if (item.ValueType() != JsonValueType::String) continue;
            for (auto known : PERMISSIONS) {
                if (item.GetString() == known) permissions.Append(item);
            }
        }
    }

    result.SetNamedValue(L"permissions", permissions);
    result.SetNamedValue(L"superAdmin", JsonValue::CreateBooleanValue(jsonBool(user, L"superAdmin")));
    return result;
}

std::wstring protect(std::wstring const& value) {
    auto utf8 = toUtf8(value);
    DATA_BLOB input{static_cast<DWORD>(utf8.size()), reinterpret_cast<BYTE*>(utf8.data())};
    DATA_BLOB output{};

    if (!CryptProtectData(&input, L"Fileship", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output))
        return {};

    auto result = base64(output.pbData, output.cbData);
    LocalFree(output.pbData);
    return result;
}

std::optional<std::wstring> unprotect(std::wstring const& value) {
    auto bytes = fromBase64(value);
    if (bytes.empty()) return std::nullopt;

    DATA_BLOB input{static_cast<DWORD>(bytes.size()), bytes.data()};
    DATA_BLOB output{};
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output))
        return std::nullopt;

    std::wstring result = toWide(std::string_view(reinterpret_cast<char*>(output.pbData), output.cbData));
    SecureZeroMemory(output.pbData, output.cbData);
    LocalFree(output.pbData);
    return result;
}

std::optional<std::vector<uint8_t>> electronKey() {
    static std::optional<std::vector<uint8_t>> key = [] {
        auto state = readJsonFile(userDataPath() / L"Local State");
        auto encrypted = fromBase64(jsonString(jsonObject(state, L"os_crypt"), L"encrypted_key"));
        if (encrypted.size() <= 5 || std::string(encrypted.begin(), encrypted.begin() + 5) != "DPAPI")
            return std::optional<std::vector<uint8_t>>();

        DATA_BLOB input{static_cast<DWORD>(encrypted.size() - 5), encrypted.data() + 5};
        DATA_BLOB output{};
        if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output))
            return std::optional<std::vector<uint8_t>>();

        std::vector<uint8_t> result(output.pbData, output.pbData + output.cbData);
        LocalFree(output.pbData);
        return std::optional(result);
    }();

    return key;
}

std::optional<std::string> decryptGcm(std::vector<uint8_t> const& key, std::vector<uint8_t> const& payload) {
    constexpr size_t NONCE = 12;
    constexpr size_t TAG = 16;
    if (payload.size() < 3 + NONCE + TAG || std::string(payload.begin(), payload.begin() + 3) != "v10")
        return std::nullopt;

    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_KEY_HANDLE handle = nullptr;
    std::optional<std::string> result;

    if (BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_AES_ALGORITHM, nullptr, 0)) &&
        BCRYPT_SUCCESS(BCryptSetProperty(algorithm, BCRYPT_CHAINING_MODE,
                                         reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(BCRYPT_CHAIN_MODE_GCM)),
                                         sizeof(BCRYPT_CHAIN_MODE_GCM), 0)) &&
        BCRYPT_SUCCESS(BCryptGenerateSymmetricKey(algorithm, &handle, nullptr, 0, const_cast<PUCHAR>(key.data()),
                                                  static_cast<ULONG>(key.size()), 0))) {
        std::vector<uint8_t> nonce(payload.begin() + 3, payload.begin() + 3 + NONCE);
        std::vector<uint8_t> cipher(payload.begin() + 3 + NONCE, payload.end() - TAG);
        std::vector<uint8_t> tag(payload.end() - TAG, payload.end());
        std::vector<uint8_t> plain(cipher.size());

        BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO info;
        BCRYPT_INIT_AUTH_MODE_INFO(info);
        info.pbNonce = nonce.data();
        info.cbNonce = static_cast<ULONG>(nonce.size());
        info.pbTag = tag.data();
        info.cbTag = static_cast<ULONG>(tag.size());

        ULONG written = 0;
        if (BCRYPT_SUCCESS(BCryptDecrypt(handle, cipher.data(), static_cast<ULONG>(cipher.size()), &info, nullptr,
                                         0, plain.data(), static_cast<ULONG>(plain.size()), &written, 0)))
            result = std::string(plain.begin(), plain.begin() + written);
    }

    if (handle) BCryptDestroyKey(handle);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    return result;
}

void save() {
    if (!writeFileAtomic(storePath(), prettyJson(data))) logError(L"Could not save the settings");
}

void migrateHarbor(JsonObject& stored) {
    if (stored.HasKey(L"driveRoot")) {
        if (!stored.HasKey(L"harborRoot")) stored.SetNamedValue(L"harborRoot", stored.GetNamedValue(L"driveRoot"));
        stored.Remove(L"driveRoot");
    }

    auto settingsValue = jsonObject(stored, L"settings");
    if (settingsValue.HasKey(L"drive")) {
        if (!settingsValue.HasKey(L"harbor")) settingsValue.SetNamedValue(L"harbor", settingsValue.GetNamedValue(L"drive"));
        settingsValue.Remove(L"drive");
        stored.SetNamedValue(L"settings", settingsValue);
    }
}

bool migrateSecret(JsonObject& stored, wchar_t const* legacy, wchar_t const* field) {
    auto old = jsonString(stored, legacy);
    if (old.empty()) return false;

    if (jsonString(stored, field).empty()) {
        if (auto plain = unprotectElectronValue(old)) {
            stored.SetNamedValue(field, JsonValue::CreateStringValue(protect(toWide(*plain))));
            logInfo(std::wstring(L"Moved the Electron ") + legacy + L" to DPAPI");
        } else {
            logWarn(std::wstring(L"Could not decrypt the Electron ") + legacy);
        }
    }

    stored.SetNamedValue(legacy, JsonValue::CreateNullValue());
    return true;
}

JsonObject normalizeStore(JsonObject stored) {
    migrateHarbor(stored);

    JsonObject result;
    for (auto const& pair : stored) result.SetNamedValue(pair.Key(), pair.Value());

    auto nullableString = [&](wchar_t const* key) {
        auto value = jsonString(stored, key);
        result.SetNamedValue(key, value.empty() ? JsonValue::CreateNullValue() : JsonValue::CreateStringValue(value));
    };

    nullableString(L"instanceUrl");
    nullableString(L"token");
    nullableString(L"uploadPassword");
    nullableString(L"harborRoot");
    nullableString(L"protectedToken");
    nullableString(L"protectedUploadPassword");
    result.SetNamedValue(L"siteName", JsonValue::CreateStringValue(jsonString(stored, L"siteName", L"Fileship")));

    auto user = stored.HasKey(L"user") ? normalizeUser(stored.GetNamedValue(L"user")) : std::nullopt;
    result.SetNamedValue(L"user", user ? IJsonValue(*user) : IJsonValue(JsonValue::CreateNullValue()));
    result.SetNamedValue(L"settings", normalizeSettings(stored.HasKey(L"settings") ? IJsonValue(stored.GetNamedValue(L"settings"))
                                                                                   : IJsonValue(JsonObject()),
                                                        false));

    auto probe = jsonObject(stored, L"ffmpegProbe");
    bool isProbe = probe.HasKey(L"key") && probe.HasKey(L"version") && probe.HasKey(L"ddagrab") &&
                   probe.HasKey(L"encoders");
    result.SetNamedValue(L"ffmpegProbe", isProbe ? IJsonValue(probe) : IJsonValue(JsonValue::CreateNullValue()));

    return result;
}

JsonObject withoutPassword(JsonObject settingsValue) {
    auto upload = jsonObject(settingsValue, L"upload");
    upload.SetNamedValue(L"password", JsonValue::CreateStringValue(L""));
    settingsValue.SetNamedValue(L"upload", upload);
    return settingsValue;
}

}

std::optional<std::string> unprotectElectronValue(std::wstring const& value) {
    auto key = electronKey();
    if (!key) return std::nullopt;
    return decryptGcm(*key, fromBase64(value));
}

void loadStore() {
    std::lock_guard lock(mutex);

    auto stored = readJsonFile(storePath());
    bool isChanged = false;

    isChanged |= migrateSecret(stored, L"token", L"protectedToken");
    isChanged |= migrateSecret(stored, L"uploadPassword", L"protectedUploadPassword");

    data = normalizeStore(stored);

    auto settingsValue = data.GetNamedObject(L"settings");
    auto password = jsonString(jsonObject(settingsValue, L"upload"), L"password");
    if (!password.empty()) {
        data.SetNamedValue(L"protectedUploadPassword", JsonValue::CreateStringValue(protect(password)));
        data.SetNamedValue(L"settings", withoutPassword(settingsValue));
        isChanged = true;
    }

    if (isChanged) save();
}

JsonObject storeData() {
    std::lock_guard lock(mutex);
    return cloneObject(data);
}

JsonObject settings() {
    std::lock_guard lock(mutex);

    auto result = cloneObject(data.GetNamedObject(L"settings"));
    auto stored = jsonString(data, L"protectedUploadPassword");
    if (stored.empty()) return result;

    auto upload = result.GetNamedObject(L"upload");
    upload.SetNamedValue(L"password", JsonValue::CreateStringValue(unprotect(stored).value_or(L"")));
    result.SetNamedValue(L"upload", upload);
    return result;
}

JsonObject updateSettings(JsonObject const& patch) {
    std::lock_guard lock(mutex);

    auto merged = settings();
    for (auto const& pair : patch) merged.SetNamedValue(pair.Key(), pair.Value());

    JsonObject next;
    try {
        next = normalizeSettings(merged, true);
    } catch (InvalidSetting const& error) {
        throw std::runtime_error(error.what());
    }

    auto password = jsonString(jsonObject(next, L"upload"), L"password");
    data.SetNamedValue(L"settings", withoutPassword(cloneObject(next)));
    data.SetNamedValue(L"protectedUploadPassword", stringOrNull(password.empty() ? L"" : protect(password)));
    save();

    return next;
}

JsonValue storeValue(std::wstring const& key) {
    std::lock_guard lock(mutex);
    if (!data.HasKey(key)) return JsonValue::CreateNullValue();
    return JsonValue::Parse(data.GetNamedValue(key).Stringify());
}

void setStoreValue(std::wstring const& key, JsonValue const& value) {
    std::lock_guard lock(mutex);
    data.SetNamedValue(key, JsonValue::Parse(value.Stringify()));
    save();
}

std::wstring instanceUrl() {
    std::lock_guard lock(mutex);
    return jsonString(data, L"instanceUrl");
}

std::wstring token() {
    std::lock_guard lock(mutex);
    auto stored = jsonString(data, L"protectedToken");
    return stored.empty() ? std::wstring() : unprotect(stored).value_or(L"");
}

std::wstring siteName() {
    std::lock_guard lock(mutex);
    return jsonString(data, L"siteName", L"Fileship");
}

std::optional<JsonObject> currentUser() {
    std::lock_guard lock(mutex);
    auto value = data.GetNamedValue(L"user");
    if (value.ValueType() != JsonValueType::Object) return std::nullopt;
    return cloneObject(value.GetObject());
}

bool isConnected() {
    std::lock_guard lock(mutex);
    return !jsonString(data, L"protectedToken").empty();
}

bool hasPermission(std::wstring const& permission) {
    auto user = currentUser();
    if (!user) return false;
    if (jsonBool(*user, L"superAdmin")) return true;

    for (auto const& item : user->GetNamedArray(L"permissions")) {
        if (item.GetString() == permission || item.GetString() == L"Admin") return true;
    }

    return false;
}

std::filesystem::path harborRoot() {
    std::lock_guard lock(mutex);
    return jsonString(data, L"harborRoot");
}

void setHarborRoot(std::filesystem::path const& root) {
    setShellHarborRoot(root.empty() ? std::nullopt : std::optional(root));
}

void setSession(std::wstring const& url, std::wstring const& name, std::wstring const& sessionToken,
                JsonObject const& user) {
    std::lock_guard lock(mutex);
    auto normalized = normalizeUser(user);

    data.SetNamedValue(L"instanceUrl", JsonValue::CreateStringValue(url));
    data.SetNamedValue(L"siteName", JsonValue::CreateStringValue(name));
    data.SetNamedValue(L"protectedToken", JsonValue::CreateStringValue(protect(sessionToken)));
    data.SetNamedValue(L"user", normalized ? IJsonValue(*normalized) : IJsonValue(JsonValue::CreateNullValue()));
    save();
}

void clearSession() {
    std::lock_guard lock(mutex);
    data.SetNamedValue(L"protectedToken", JsonValue::CreateNullValue());
    data.SetNamedValue(L"user", JsonValue::CreateNullValue());
    save();
}

}
