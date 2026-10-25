#include "app.h"
#include "rpc.h"

#include <host/harbor.h>
#include <shared/log.h>

#include <mutex>

namespace fileship::host {

namespace {

constexpr int64_t CHECK_REUSE_MS = 5 * 60 * 1000;
constexpr wchar_t const* MIN_FILESHIP_VERSION = L"6.0.0";

std::mutex checkMutex;
std::optional<std::pair<JsonObject, int64_t>> lastCheck;

ApiError readError(HttpResponse const& response) {
    auto body = parseObject(response.text());
    auto message = body ? jsonString(*body, L"message") : std::wstring();
    if (message.empty()) message = t(L"app.requestFailed", {{L"status", std::to_wstring(response.status)}});

    return ApiError(toUtf8(message), response.status, body ? jsonObject(*body, L"data") : JsonObject());
}

struct Version {
    int major = 0;
    int minor = 0;
    int patch = 0;
};

std::optional<Version> coerceVersion(std::wstring const& text) {
    size_t start = text.find_first_of(L"0123456789");
    if (start == std::wstring::npos) return std::nullopt;

    Version version;
    int* parts[] = {&version.major, &version.minor, &version.patch};
    size_t index = start;

    for (size_t part = 0; part < 3; part++) {
        if (index >= text.size() || !iswdigit(text[index])) break;

        size_t end = index;
        while (end < text.size() && iswdigit(text[end])) end++;
        *parts[part] = std::stoi(text.substr(index, end - index));
        if (end >= text.size() || text[end] != L'.') break;
        index = end + 1;
    }

    return version;
}

std::wstring versionText(Version const& version) {
    return std::to_wstring(version.major) + L"." + std::to_wstring(version.minor) + L"." +
           std::to_wstring(version.patch);
}

bool isOlder(Version const& a, Version const& b) {
    if (a.major != b.major) return a.major < b.major;
    if (a.minor != b.minor) return a.minor < b.minor;
    return a.patch < b.patch;
}

JsonObject fetchUser(std::wstring const& url, std::wstring const& sessionToken) {
    auto user = request(url, L"GET", L"/api/users/@me", {}, {}, sessionToken);
    if (user.ValueType() != JsonValueType::Object) throw ApiError("Invalid user", 500, JsonObject());
    return user.GetObject();
}

JsonObject checkedInstance(std::wstring const& input) {
    {
        std::lock_guard lock(checkMutex);
        if (lastCheck && jsonString(lastCheck->first, L"url") == input && nowMs() - lastCheck->second < CHECK_REUSE_MS)
            return lastCheck->first;
    }

    return checkInstance(input);
}

bool canUseDesktop(JsonObject const& user) {
    if (jsonBool(user, L"superAdmin")) return true;
    if (!user.HasKey(L"permissions") || user.GetNamedValue(L"permissions").ValueType() != JsonValueType::Array)
        return false;

    for (auto const& item : user.GetNamedArray(L"permissions")) {
        if (item.ValueType() != JsonValueType::String) continue;
        std::wstring permission(item.GetString());
        if (permission == L"Admin" || permission == L"UploadFiles" || permission == L"ShortenLinks") return true;
    }

    return false;
}

}

JsonValue request(std::wstring const& baseUrl, std::wstring const& method, std::wstring const& path,
                  std::vector<char> body, std::wstring const& contentType, std::wstring const& bearer,
                  int timeoutMs) {
    HttpEndpoint endpoint{baseUrl, bearer, desktopUserAgent(), language()};
    HttpRequest options{method, path, contentType.empty() ? L"" : L"Content-Type: " + contentType + L"\r\n",
                        std::move(body), timeoutMs};

    HttpResponse response;

    try {
        response = httpSend(endpoint, options);
    } catch (std::exception const& error) {
        std::string what = error.what();
        bool isTimeout = what.find("12002") != std::string::npos;

        throw ApiError(toUtf8(isTimeout ? t(L"app.serverTimeout") : t(L"app.serverUnreachable", {{L"url", baseUrl}})),
                       0, JsonObject());
    }

    if (response.status < 200 || response.status >= 300) throw readError(response);

    auto text = response.text();
    if (text.empty()) return JsonValue::CreateNullValue();

    JsonValue value{nullptr};
    if (!JsonValue::TryParse(toWide(text), value)) throw ApiError("Invalid response", response.status, JsonObject());
    return value;
}

HttpEndpoint apiEndpoint() {
    return HttpEndpoint{instanceUrl(), token(), desktopUserAgent(), language()};
}

JsonValue api(std::wstring const& method, std::wstring const& path, std::optional<IJsonValue> const& body,
              int timeoutMs) {
    auto url = instanceUrl();
    if (url.empty()) throw ApiError(toUtf8(t(L"app.notConnected")), 401, JsonObject());

    std::vector<char> bytes;
    if (body) {
        auto text = toUtf8(std::wstring(body->Stringify()));
        bytes.assign(text.begin(), text.end());
    }

    return request(url, method, path, std::move(bytes), body ? L"application/json" : L"", token(), timeoutMs);
}

JsonValue apiForm(std::wstring const& path, Multipart& form, int timeoutMs) {
    auto url = instanceUrl();
    if (url.empty()) throw ApiError(toUtf8(t(L"app.notConnected")), 401, JsonObject());

    auto contentType = form.contentType();
    return request(url, L"POST", path, form.finish(), contentType, token(), timeoutMs);
}

std::wstring normalizeInstanceUrl(std::wstring const& input) {
    auto start = input.find_first_not_of(L" \t\r\n");
    auto end = input.find_last_not_of(L" \t\r\n");
    if (start == std::wstring::npos) throw translatedError(t(L"app.invalidAddress"));

    auto trimmed = input.substr(start, end - start + 1);
    auto lower = trimmed;
    for (auto& character : lower) character = static_cast<wchar_t>(towlower(character));
    if (lower.rfind(L"http://", 0) != 0 && lower.rfind(L"https://", 0) != 0) trimmed = L"https://" + trimmed;

    URL_COMPONENTS url{};
    url.dwStructSize = sizeof(url);
    wchar_t host[256]{};
    url.lpszHostName = host;
    url.dwHostNameLength = static_cast<DWORD>(std::size(host));
    url.dwUrlPathLength = static_cast<DWORD>(-1);

    if (!WinHttpCrackUrl(trimmed.c_str(), 0, 0, &url) || !host[0]) throw translatedError(t(L"app.invalidAddress"));

    std::wstring hostName(host);
    for (auto& character : hostName) character = static_cast<wchar_t>(towlower(character));

    bool isHttps = url.nScheme == INTERNET_SCHEME_HTTPS;
    bool isDefaultPort = url.nPort == (isHttps ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT);

    return std::wstring(isHttps ? L"https://" : L"http://") + hostName +
           (isDefaultPort ? L"" : L":" + std::to_wstring(url.nPort));
}

JsonObject checkInstance(std::wstring const& input) {
    auto url = normalizeInstanceUrl(input);
    JsonValue health{nullptr};

    try {
        health = request(url, L"GET", L"/api/healthz", {}, {}, {}, 10000);
    } catch (ApiError const& error) {
        throw translatedError(error.status == 503 ? t(L"app.serverStarting")
                                                  : t(L"app.serverNotFound", {{L"url", url}}));
    }

    auto reported = health.ValueType() == JsonValueType::Object ? jsonString(health.GetObject(), L"version") : L"";
    auto version = coerceVersion(reported);
    if (!version) throw translatedError(t(L"app.serverNotFound", {{L"url", url}}));

    auto minimum = *coerceVersion(MIN_FILESHIP_VERSION);
    if (isOlder(*version, minimum)) {
        throw translatedError(t(L"app.serverTooOld",
                                {{L"version", versionText(*version)}, {L"minVersion", MIN_FILESHIP_VERSION}}));
    }

    std::wstring name = L"Fileship";
    try {
        auto publicSettings = request(url, L"GET", L"/api/settings/public");
        if (publicSettings.ValueType() == JsonValueType::Object) {
            auto value = jsonString(publicSettings.GetObject(), L"siteName");
            if (!value.empty()) name = value;
        }
    } catch (std::exception const&) {
    }

    JsonObject info;
    info.SetNamedValue(L"url", JsonValue::CreateStringValue(url));
    info.SetNamedValue(L"version", JsonValue::CreateStringValue(versionText(*version)));
    info.SetNamedValue(L"siteName", JsonValue::CreateStringValue(name));

    std::lock_guard lock(checkMutex);
    lastCheck = std::pair(cloneObject(info), nowMs());
    return info;
}

IJsonValue accountJson() {
    auto store = storeData();
    auto url = jsonString(store, L"instanceUrl");
    auto user = store.GetNamedValue(L"user");
    if (url.empty() || user.ValueType() != JsonValueType::Object) return JsonValue::CreateNullValue();

    JsonObject account;
    account.SetNamedValue(L"instanceUrl", JsonValue::CreateStringValue(url));
    account.SetNamedValue(L"siteName", JsonValue::CreateStringValue(jsonString(store, L"siteName", L"Fileship")));
    account.SetNamedValue(L"user", user);
    return account;
}

IJsonValue getAccount() {
    auto url = instanceUrl();
    auto sessionToken = token();
    if (url.empty() || sessionToken.empty()) return JsonValue::CreateNullValue();

    try {
        setSession(url, siteName(), sessionToken, fetchUser(url, sessionToken));
    } catch (ApiError const& error) {
        if (error.status == 401) {
            handleUnauthorized();
            return JsonValue::CreateNullValue();
        }

        logWarn(L"Could not refresh the account, using the cached one: " + errorMessage(error));
    }

    return accountJson();
}

IJsonValue startSession(std::wstring const& url, std::wstring const& sessionToken) {
    auto instance = checkedInstance(normalizeInstanceUrl(url));
    auto origin = jsonString(instance, L"url");
    auto user = fetchUser(origin, sessionToken);

    if (!canUseDesktop(user)) {
        try {
            request(origin, L"POST", L"/api/auth/logout", {}, {}, sessionToken);
        } catch (std::exception const&) {
        }

        throw translatedError(t(L"app.noUploadPermission"));
    }

    setSession(origin, jsonString(instance, L"siteName", L"Fileship"), sessionToken, user);

    auto account = accountJson();
    broadcast(L"account:changed", account);
    resetUploadSettings();
    refreshTray();
    harbor::startHarbor();
    return account;
}

void endSession() {
    harbor::stopHarbor({true, true});

    auto url = instanceUrl();
    auto sessionToken = token();

    if (!url.empty() && !sessionToken.empty()) {
        try {
            request(url, L"POST", L"/api/auth/logout", {}, {}, sessionToken);
        } catch (std::exception const&) {
        }
    }

    clearSession();
    broadcast(L"account:changed", JsonValue::CreateNullValue());
    refreshTray();
}

void handleUnauthorized() {
    if (!isConnected()) return;

    clearSession();
    broadcast(L"account:changed", JsonValue::CreateNullValue());
    refreshTray();
    runAsync([] { harbor::stopHarbor({true, true}); });
}

}
