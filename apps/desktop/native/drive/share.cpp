#include "com.h"
#include "share.h"

#include <shlobj.h>
#include <shlwapi.h>

using namespace winrt;

namespace drive {

CLSID const CLSID_FileshipShare{0x5904efec, 0xd576, 0x4002, {0x9f, 0x5a, 0xde, 0xeb, 0x56, 0x33, 0x4d, 0xf9}};

namespace {

std::vector<std::wstring> pathsOf(IShellItemArray* items) {
    std::vector<std::wstring> paths;
    DWORD count = 0;
    if (!items || FAILED(items->GetCount(&count))) return paths;

    for (DWORD index = 0; index < count; index++) {
        com_ptr<IShellItem> item;
        if (FAILED(items->GetItemAt(index, item.put()))) continue;

        wchar_t* name = nullptr;
        if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &name))) continue;

        paths.emplace_back(name);
        CoTaskMemFree(name);
    }

    return paths;
}

struct Share : implements<Share, IExplorerCommand> {
    ServerLock lock;

    HRESULT __stdcall GetTitle(IShellItemArray*, LPWSTR* name) noexcept final {
        return SHStrDupW(label(L"copyLink").c_str(), name);
    }

    HRESULT __stdcall GetIcon(IShellItemArray*, LPWSTR* icon) noexcept final {
        try {
            return SHStrDupW((executablePath() + L",0").c_str(), icon);
        } catch (...) {
            return to_hresult();
        }
    }

    HRESULT __stdcall GetToolTip(IShellItemArray*, LPWSTR* tip) noexcept final {
        *tip = nullptr;
        return E_NOTIMPL;
    }

    HRESULT __stdcall GetCanonicalName(GUID* name) noexcept final {
        *name = CLSID_FileshipShare;
        return S_OK;
    }

    HRESULT __stdcall GetState(IShellItemArray*, BOOL, EXPCMDSTATE* state) noexcept final {
        *state = ECS_ENABLED;
        return S_OK;
    }

    HRESULT __stdcall Invoke(IShellItemArray* items, IBindCtx*) noexcept final {
        try {
            auto paths = pathsOf(items);
            if (paths.empty()) return E_INVALIDARG;

            requestAction(L"copyLink", paths);
            return S_OK;
        } catch (...) {
            return to_hresult();
        }
    }

    HRESULT __stdcall GetFlags(EXPCMDFLAGS* flags) noexcept final {
        *flags = ECF_DEFAULT;
        return S_OK;
    }

    HRESULT __stdcall EnumSubCommands(IEnumExplorerCommand** commands) noexcept final {
        *commands = nullptr;
        return E_NOTIMPL;
    }
};

}

com_ptr<IClassFactory> shareFactory() {
    return classFactory<Share>();
}

}
