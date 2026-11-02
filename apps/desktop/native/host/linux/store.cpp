#include <host/app.h>
#include <shared/log.h>

#include <libsecret/secret.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

namespace fileship::host {

namespace {

constexpr wchar_t const* KEYRING = L"keyring";
constexpr wchar_t const* PLAIN = L"plain:";
constexpr auto KEYRING_TIMEOUT = std::chrono::minutes(2);
constexpr auto CANCEL_TIMEOUT = std::chrono::seconds(5);

std::atomic<bool> isRefused = false;

struct Outcome {
    bool isDone = false;
    bool isOk = false;
    std::string value;
    std::string error;
};

struct Call {
    std::mutex mutex;
    std::condition_variable condition;
    Outcome outcome;
    GCancellable* cancellable = g_cancellable_new();

    ~Call() {
        g_object_unref(cancellable);
    }
};

Outcome runWithDeadline(std::function<void(GCancellable*, Outcome&)> work) {
    auto call = std::make_shared<Call>();

    std::thread([call, work = std::move(work)] {
        Outcome outcome;
        work(call->cancellable, outcome);

        {
            std::lock_guard lock(call->mutex);
            outcome.isDone = true;
            call->outcome = std::move(outcome);
        }
        call->condition.notify_all();
    }).detach();

    std::unique_lock lock(call->mutex);
    auto isDone = [&] { return call->outcome.isDone; };
    if (!call->condition.wait_for(lock, KEYRING_TIMEOUT, isDone)) {
        g_cancellable_cancel(call->cancellable);
        call->condition.wait_for(lock, CANCEL_TIMEOUT, isDone);
    }

    return call->outcome;
}

std::string describe(Outcome const& outcome, char const* fallback) {
    if (!outcome.isDone) return "the keyring did not answer";
    return outcome.error.empty() ? fallback : outcome.error;
}

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
    auto utf8 = toUtf8(value);

    if (!isRefused) {
        auto outcome = runWithDeadline([utf8, key = toUtf8(name), owner = profile()](GCancellable* cancellable,
                                                                                     Outcome& result) {
            GError* error = nullptr;
            result.isOk = secret_password_store_sync(schema(), SECRET_COLLECTION_DEFAULT, "Fileship", utf8.c_str(),
                                                     cancellable, &error, "profile", owner.c_str(), "name",
                                                     key.c_str(), nullptr);
            if (error) {
                result.error = error->message;
                g_error_free(error);
            }
        });
        if (outcome.isOk) return KEYRING;

        isRefused = true;
        logWarn(L"The keyring did not store " + name + L", keeping it in the settings file: " +
                toWide(describe(outcome, "the prompt was dismissed")));
    }

    return PLAIN + base64(utf8.data(), utf8.size());
}

std::optional<std::wstring> unprotectSecret(std::wstring const& name, std::wstring const& stored) {
    if (stored.starts_with(PLAIN)) {
        auto bytes = fromBase64(stored.substr(std::wstring_view(PLAIN).size()));
        return toWide(std::string_view(reinterpret_cast<char const*>(bytes.data()), bytes.size()));
    }

    if (stored != KEYRING) return std::nullopt;

    auto outcome = runWithDeadline([key = toUtf8(name), owner = profile()](GCancellable* cancellable,
                                                                           Outcome& result) {
        GError* error = nullptr;
        auto secret = secret_password_lookup_sync(schema(), cancellable, &error, "profile", owner.c_str(), "name",
                                                  key.c_str(), nullptr);
        if (secret) {
            result.isOk = true;
            result.value = secret;
            secret_password_free(secret);
        }
        if (error) {
            result.error = error->message;
            g_error_free(error);
        }
    });

    if (outcome.isOk) return toWide(outcome.value);

    logWarn(L"Could not read " + name + L" from the keyring: " + toWide(describe(outcome, "it is not there")));
    return std::nullopt;
}

std::optional<std::string> unprotectElectronValue(std::wstring const&) {
    return std::nullopt;
}

}
