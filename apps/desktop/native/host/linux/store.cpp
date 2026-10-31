#include <host/app.h>
#include <shared/log.h>

#include <libsecret/secret.h>

namespace fileship::host {

namespace {

constexpr wchar_t const* KEYRING = L"keyring";
constexpr wchar_t const* PLAIN = L"plain:";

SecretSchema const* schema() {
    static SecretSchema const value = {
        "dev.hanzy.fileship.Secret",
        SECRET_SCHEMA_NONE,
        {{"profile", SECRET_SCHEMA_ATTRIBUTE_STRING}, {"name", SECRET_SCHEMA_ATTRIBUTE_STRING}, {nullptr, {}}},
        0,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
    };
    return &value;
}

std::string profile() {
    return userDataPath().string();
}

}

std::wstring protectSecret(std::wstring const& name, std::wstring const& value) {
    GError* error = nullptr;
    auto utf8 = toUtf8(value);

    if (secret_password_store_sync(schema(), SECRET_COLLECTION_DEFAULT, "Fileship", utf8.c_str(), nullptr, &error,
                                   "profile", profile().c_str(), "name", toUtf8(name).c_str(), nullptr))
        return KEYRING;

    logWarn(L"No keyring is available, storing " + name + L" in the settings file: " +
            toWide(error ? error->message : "unknown error"));
    if (error) g_error_free(error);
    return PLAIN + base64(utf8.data(), utf8.size());
}

std::optional<std::wstring> unprotectSecret(std::wstring const& name, std::wstring const& stored) {
    if (stored.starts_with(PLAIN)) {
        auto bytes = fromBase64(stored.substr(std::wstring_view(PLAIN).size()));
        return toWide(std::string_view(reinterpret_cast<char const*>(bytes.data()), bytes.size()));
    }

    if (stored != KEYRING) return std::nullopt;

    auto secret = secret_password_lookup_sync(schema(), nullptr, nullptr, "profile", profile().c_str(), "name",
                                              toUtf8(name).c_str(), nullptr);
    if (!secret) return std::nullopt;

    std::wstring result = toWide(secret);
    secret_password_free(secret);
    return result;
}

std::optional<std::string> unprotectElectronValue(std::wstring const&) {
    return std::nullopt;
}

}
