#include <windows.h>

#include <delayimp.h>

#include <cstring>

namespace {

FARPROC WINAPI loadHostExecutable(unsigned int event, DelayLoadInfo* info) {
    if (event != dliNotePreLoadLibrary || _stricmp(info->szDll, "node.exe") != 0) return nullptr;
    return reinterpret_cast<FARPROC>(GetModuleHandleW(nullptr));
}

}

decltype(__pfnDliNotifyHook2) __pfnDliNotifyHook2 = loadHostExecutable;
