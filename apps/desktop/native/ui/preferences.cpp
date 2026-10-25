#include "preferences.h"

#include "overlay.h"

#include <shared/json.h>
#include <shared/log.h>

#include <mutex>

namespace fileship::ui {

namespace {

std::mutex mutex;
OverlayPreferencesStore store;

wchar_t const* edgeName(ToolbarEdge edge) {
    switch (edge) {
    case ToolbarEdge::Top:
        return L"top";
    case ToolbarEdge::Bottom:
        return L"bottom";
    case ToolbarEdge::Left:
        return L"left";
    case ToolbarEdge::Right:
        return L"right";
    default:
        return nullptr;
    }
}

ToolbarEdge parseEdge(std::wstring const& value) {
    if (value == L"top") return ToolbarEdge::Top;
    if (value == L"bottom") return ToolbarEdge::Bottom;
    if (value == L"left") return ToolbarEdge::Left;
    if (value == L"right") return ToolbarEdge::Right;
    return ToolbarEdge::None;
}

bool isType(JsonObject const& object, wchar_t const* key, JsonValueType type) {
    return object.HasKey(key) && object.GetNamedValue(key).ValueType() == type;
}

}

void setOverlayPreferencesStore(OverlayPreferencesStore value) {
    std::lock_guard lock(mutex);
    store = std::move(value);
}

OverlayPreferences loadPreferences() {
    OverlayPreferences result;
    std::function<std::string()> load;
    {
        std::lock_guard lock(mutex);
        load = store.load;
    }
    if (!load) return result;

    try {
        auto object = parseObject(load());
        if (!object) return result;

        if (isType(*object, L"toolbar", JsonValueType::Object)) {
            auto toolbar = object->GetNamedObject(L"toolbar");
            if (isType(toolbar, L"display", JsonValueType::Number))
                result.toolbar.display = static_cast<int64_t>(toolbar.GetNamedNumber(L"display"));
            if (isType(toolbar, L"edge", JsonValueType::String))
                result.toolbar.edge = parseEdge(std::wstring(toolbar.GetNamedString(L"edge")));
            if (isType(toolbar, L"edge", JsonValueType::Null)) result.toolbar.edge = ToolbarEdge::None;
            if (isType(toolbar, L"cx", JsonValueType::Number)) result.toolbar.cx = toolbar.GetNamedNumber(L"cx");
            if (isType(toolbar, L"cy", JsonValueType::Number)) result.toolbar.cy = toolbar.GetNamedNumber(L"cy");
        }

        if (isType(*object, L"annotations", JsonValueType::Object)) {
            auto annotations = object->GetNamedObject(L"annotations");
            auto& options = result.annotations;

            if (isType(annotations, L"tools", JsonValueType::Object)) {
                auto tools = annotations.GetNamedObject(L"tools");
                for (auto const& info : TOOLS) {
                    auto name = toWide(info.name);
                    if (!isType(tools, name.c_str(), JsonValueType::Object)) continue;

                    auto tool = tools.GetNamedObject(name);
                    auto& target = options.tools[static_cast<size_t>(info.tool)];
                    if (isType(tool, L"color", JsonValueType::String))
                        target.color = toUtf8(std::wstring(tool.GetNamedString(L"color")));
                    if (isType(tool, L"size", JsonValueType::Number)) target.size = tool.GetNamedNumber(L"size");
                }
            }

            if (isType(annotations, L"fill", JsonValueType::Boolean)) options.fill = annotations.GetNamedBoolean(L"fill");
            if (isType(annotations, L"background", JsonValueType::Boolean))
                options.background = annotations.GetNamedBoolean(L"background");
            if (isType(annotations, L"shadow", JsonValueType::Boolean))
                options.shadow = annotations.GetNamedBoolean(L"shadow");
            if (isType(annotations, L"emoji", JsonValueType::String))
                options.emoji = std::wstring(annotations.GetNamedString(L"emoji"));
        }
    } catch (...) {
        logWarn(L"Could not read the overlay preferences");
    }

    return result;
}

void savePreferences(OverlayPreferences const& preferences) {
    std::function<void(std::string const&)> save;
    {
        std::lock_guard lock(mutex);
        save = store.save;
    }
    if (!save) return;

    JsonObject toolbar;
    toolbar.SetNamedValue(L"display", preferences.toolbar.display
                                          ? JsonValue::CreateNumberValue(static_cast<double>(*preferences.toolbar.display))
                                          : JsonValue::CreateNullValue());
    auto edge = edgeName(preferences.toolbar.edge);
    toolbar.SetNamedValue(L"edge", edge ? JsonValue::CreateStringValue(edge) : JsonValue::CreateNullValue());
    toolbar.SetNamedValue(L"cx", JsonValue::CreateNumberValue(preferences.toolbar.cx));
    toolbar.SetNamedValue(L"cy", JsonValue::CreateNumberValue(preferences.toolbar.cy));

    JsonObject tools;
    for (auto const& info : TOOLS) {
        auto const& options = preferences.annotations.tools[static_cast<size_t>(info.tool)];
        JsonObject tool;
        tool.SetNamedValue(L"color", JsonValue::CreateStringValue(toWide(options.color)));
        tool.SetNamedValue(L"size", JsonValue::CreateNumberValue(options.size));
        tools.SetNamedValue(toWide(info.name), tool);
    }

    JsonObject annotations;
    annotations.SetNamedValue(L"tools", tools);
    annotations.SetNamedValue(L"fill", JsonValue::CreateBooleanValue(preferences.annotations.fill));
    annotations.SetNamedValue(L"background", JsonValue::CreateBooleanValue(preferences.annotations.background));
    annotations.SetNamedValue(L"shadow", JsonValue::CreateBooleanValue(preferences.annotations.shadow));
    annotations.SetNamedValue(L"emoji", JsonValue::CreateStringValue(preferences.annotations.emoji));

    JsonObject object;
    object.SetNamedValue(L"toolbar", toolbar);
    object.SetNamedValue(L"annotations", annotations);

    try {
        save(stringify(object));
    } catch (...) {
        logWarn(L"Could not save the overlay preferences");
    }
}

}
