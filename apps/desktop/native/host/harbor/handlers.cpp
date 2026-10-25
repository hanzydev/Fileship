#include "engine.h"
#include "support.h"

#include <host/rpc.h>
#include <host/services.h>
#include <shared/i18n.h>

#undef GetObject

namespace fileship::host::harbor {

namespace {

std::wstring stringArg(JsonArray const& args, uint32_t index) {
    if (args.Size() <= index || args.GetAt(index).ValueType() != JsonValueType::String) return L"";
    return std::wstring(args.GetAt(index).GetString());
}

bool boolArg(JsonArray const& args, uint32_t index) {
    return args.Size() > index && args.GetAt(index).ValueType() == JsonValueType::Boolean &&
           args.GetAt(index).GetBoolean();
}

std::wstring resolved(std::filesystem::path const& path) {
    std::error_code error;
    auto absolute = std::filesystem::absolute(path, error).lexically_normal().wstring();
    while (absolute.size() > 3 && (absolute.back() == L'\\' || absolute.back() == L'/')) absolute.pop_back();
    return lower(absolute);
}

JsonValue statusValue() {
    return JsonValue::Parse(harborStatus().Stringify());
}

JsonValue setFolder(std::wstring const& folder) {
    if (!folder.empty() && resolved(folder) == resolved(harborFolder())) return statusValue();

    std::error_code error;
    if (!folder.empty() && std::filesystem::exists(folder, error) &&
        std::filesystem::directory_iterator(folder, error) != std::filesystem::directory_iterator()) {
        throw std::runtime_error(toUtf8(t(L"app.harborFolderNotEmpty")));
    }

    stopHarbor({.remove = true});

    auto current = settings();
    JsonObject harbor = current && current.HasKey(L"harbor") ? current.GetNamedObject(L"harbor") : JsonObject();
    harbor.SetNamedValue(L"folder", JsonValue::CreateStringValue(folder));

    JsonObject patch;
    patch.SetNamedValue(L"harbor", harbor);
    updateSettings(patch);

    startHarborAndWait();
    return statusValue();
}

JsonValue breakdown() {
    auto stats = api(L"GET", L"/api/users/@me/stats").GetObject();
    JsonArray result;

    for (auto const& value : stats.GetNamedArray(L"byType", JsonArray())) {
        auto item = value.GetObject();
        JsonObject entry;
        entry.SetNamedValue(L"type", JsonValue::CreateStringValue(stringField(item, L"type").value_or(L"")));
        entry.SetNamedValue(L"size", JsonValue::CreateNumberValue(numberField(item, L"size").value_or(0)));
        result.Append(entry);
    }

    return JsonValue::Parse(result.Stringify());
}

}

void registerHarborHandlers() {
    registerHandler(L"harbor:status", [](JsonArray const&) { return statusValue(); });
    registerHandler(L"harbor:open", [](JsonArray const&) {
        openHarbor();
        return JsonValue::CreateNullValue();
    });
    registerHandler(L"harbor:setFolder", [](JsonArray const& args) { return setFolder(stringArg(args, 0)); });
    registerHandler(L"harbor:pause", [](JsonArray const& args) {
        pauseHarbor(boolArg(args, 0));
        return statusValue();
    });
    registerHandler(L"harbor:freeUpSpace", [](JsonArray const&) {
        return JsonValue::CreateNumberValue(static_cast<double>(freeUpHarborSpace()));
    });
    registerHandler(L"harbor:activity", [](JsonArray const&) {
        return JsonValue::Parse(harborActivity().Stringify());
    });
    registerHandler(L"harbor:breakdown", [](JsonArray const&) { return breakdown(); });
}

}
