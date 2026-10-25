#pragma once

#include <shared/json.h>

#include <cstdint>
#include <functional>
#include <string>

namespace fileship::host {

using winrt::Windows::Data::Json::IJsonValue;

using RpcHandler = std::function<IJsonValue(JsonArray const& args)>;

void registerHandler(std::wstring const& channel, RpcHandler handler);
void broadcast(std::wstring const& event, IJsonValue const& payload);

uint64_t currentClient();
void sendTo(uint64_t client, std::wstring const& event, IJsonValue const& payload);
void onClientClosed(std::function<void(uint64_t client)> listener);
bool hasWindowClient();

}
