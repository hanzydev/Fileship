#pragma once

#include "common.h"

namespace drive {

void serverAddRef();
void serverRelease();

void registerClassObjects();
void revokeClassObjects();
void writeShellRegistration();
void removeShellRegistration();
void waitUntilIdle();

void saveSession();
bool loadSession();
void removeSession();

}
