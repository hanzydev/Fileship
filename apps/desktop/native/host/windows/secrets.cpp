#include "host.h"

#include <host/platform.h>
#include <shared/log.h>

#include <bcrypt.h>
#include <dpapi.h>

namespace fileship::host {

namespace {

JsonObject readJsonFile(std::filesystem::path const& path) {
    auto bytes = readFileBytes(path);
    auto object = parseObject(std::string_view(bytes.data(), bytes.size()));
    return object ? *object : JsonObject();
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

std::optional<std::string> unprotectElectronValue(std::wstring const& value) {
    auto key = electronKey();
    if (!key) return std::nullopt;
    return decryptGcm(*key, fromBase64(value));
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

}

std::wstring storeSecret(std::wstring const&, std::wstring const& value) {
    return value.empty() ? std::wstring() : protect(value);
}

std::wstring loadSecret(std::wstring const&, std::wstring const& stored) {
    return unprotect(stored).value_or(L"");
}

void migrateElectronStore() {
    auto path = userDataPath() / L"settings.json";
    auto stored = readJsonFile(path);

    bool isChanged = migrateSecret(stored, L"token", L"protectedToken");
    isChanged |= migrateSecret(stored, L"uploadPassword", L"protectedUploadPassword");

    if (isChanged && !writeFileAtomic(path, prettyJson(stored))) logError(L"Could not save the settings");
}

}
