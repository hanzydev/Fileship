#include "services.h"

#include <shared/i18n.h>
#include <shared/log.h>

#include <windows.h>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    fileship::initLog(fileship::host::userDataPath() / L"logs" / L"host.log");
    fileship::initI18n(fileship::host::resourcesPath() / L"i18n");
    fileship::logInfo(L"Fileship host " + fileship::host::appVersion());
    return 0;
}
