#pragma once

#include <host/app.h>

#include <gio/gio.h>

namespace fileship::host {

constexpr char const* APP_ID = "dev.hanzy.fileship";

GDBusConnection* sessionBus();

void initNotifications();

}
