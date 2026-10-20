#pragma once

#include "common.h"

namespace drive {

void serverAddRef();
void serverRelease();

struct ServerLock {
    ServerLock() {
        serverAddRef();
    }

    ~ServerLock() {
        serverRelease();
    }

    ServerLock(ServerLock const&) = delete;
    ServerLock& operator=(ServerLock const&) = delete;
};

template <typename T>
struct ClassFactory : winrt::implements<ClassFactory<T>, IClassFactory> {
    HRESULT __stdcall CreateInstance(IUnknown* outer, REFIID iid, void** result) noexcept final {
        *result = nullptr;
        if (outer) return CLASS_E_NOAGGREGATION;

        try {
            return winrt::make_self<T>()->QueryInterface(iid, result);
        } catch (...) {
            return winrt::to_hresult();
        }
    }

    HRESULT __stdcall LockServer(BOOL) noexcept final {
        return S_OK;
    }
};

template <typename T>
winrt::com_ptr<IClassFactory> classFactory() {
    return winrt::make<ClassFactory<T>>().template as<IClassFactory>();
}

std::wstring executablePath();

void registerClassObjects();
void revokeClassObjects();
void writeShellRegistration();
void removeShellRegistration();
void waitUntilIdle();

void setHosted();
void requestAction(std::wstring const& action, std::vector<std::wstring> const& paths);

void saveSession();
bool loadSession();
void removeSession();

}
