#include <host/app.h>
#include <host/platform.h>

#include <libsecret/secret.h>

namespace fileship::host {

namespace {

SecretSchema const SCHEMA = [] {
    SecretSchema schema{};
    schema.name = "dev.hanzy.fileship";
    schema.flags = SECRET_SCHEMA_NONE;
    schema.attributes[0] = {"name", SECRET_SCHEMA_ATTRIBUTE_STRING};
    schema.attributes[1] = {"profile", SECRET_SCHEMA_ATTRIBUTE_STRING};
    return schema;
}();

[[noreturn]] void fail(GError* error) {
    std::string message = error->message;
    g_error_free(error);
    throw std::runtime_error("The keyring refused: " + message);
}

}

std::wstring storeSecret(std::wstring const& name, std::wstring const& value) {
    auto key = toUtf8(name);
    auto profile = userDataPath().string();
    GError* error = nullptr;

    if (value.empty()) {
        secret_password_clear_sync(&SCHEMA, nullptr, &error, "name", key.c_str(), "profile", profile.c_str(), nullptr);
        g_clear_error(&error);
        return {};
    }

    secret_password_store_sync(&SCHEMA, SECRET_COLLECTION_DEFAULT, ("Fileship " + key).c_str(), toUtf8(value).c_str(),
                               nullptr, &error, "name", key.c_str(), "profile", profile.c_str(), nullptr);
    if (error) fail(error);

    return L"keyring";
}

std::wstring loadSecret(std::wstring const& name, std::wstring const&) {
    auto profile = userDataPath().string();
    GError* error = nullptr;

    gchar* value = secret_password_lookup_sync(&SCHEMA, nullptr, &error, "name", toUtf8(name).c_str(), "profile",
                                               profile.c_str(), nullptr);
    if (error) fail(error);
    if (!value) return {};

    auto result = toWide(value);
    secret_password_free(value);
    return result;
}

}
