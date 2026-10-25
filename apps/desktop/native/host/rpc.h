#pragma once

#include <shared/json.h>

#include <functional>
#include <string>

namespace fileship::host {

using RpcHandler = std::function<JsonValue(JsonArray const& args)>;

void registerHandler(std::wstring const& channel, RpcHandler handler);
void broadcast(std::wstring const& event, JsonValue const& payload);

}
