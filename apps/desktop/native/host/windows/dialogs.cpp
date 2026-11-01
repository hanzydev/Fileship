#include <host/app.h>

#include <windows.h>
#include <commctrl.h>
#include <objbase.h>
#include <shobjidl.h>
#include <winrt/base.h>

namespace fileship::host {

namespace {

HRESULT CALLBACK dialogCallback(HWND window, UINT notification, WPARAM, LPARAM, LONG_PTR) {
    if (notification == TDN_CREATED) {
        SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetForegroundWindow(window);
    }

    return S_OK;
}

}

bool confirmDialog(std::wstring const& message, std::wstring const& detail, std::wstring const& accept,
                   std::wstring const& cancel) {
    constexpr int ACCEPT = 100;
    constexpr int CANCEL = 101;

    TASKDIALOG_BUTTON buttons[] = {{ACCEPT, accept.c_str()}, {CANCEL, cancel.c_str()}};

    TASKDIALOGCONFIG config{};
    config.cbSize = sizeof(config);
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION;
    config.pszWindowTitle = L"Fileship";
    config.pszMainInstruction = message.c_str();
    config.pszContent = detail.c_str();
    config.cButtons = static_cast<UINT>(std::size(buttons));
    config.pButtons = buttons;
    config.nDefaultButton = ACCEPT;
    config.pfCallback = dialogCallback;

    int pressed = CANCEL;
    if (FAILED(TaskDialogIndirect(&config, &pressed, nullptr, nullptr))) return false;
    return pressed == ACCEPT;
}

std::vector<std::filesystem::path> pickFiles() {
    std::vector<std::filesystem::path> paths;

    auto dialog = winrt::create_instance<IFileOpenDialog>(CLSID_FileOpenDialog);
    FILEOPENDIALOGOPTIONS options{};
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_ALLOWMULTISELECT | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST);
    auto title = t(L"app.uploadDialogTitle");
    dialog->SetTitle(title.c_str());

    if (FAILED(dialog->Show(nullptr))) return paths;

    winrt::com_ptr<IShellItemArray> items;
    if (FAILED(dialog->GetResults(items.put()))) return paths;

    DWORD count = 0;
    items->GetCount(&count);

    for (DWORD index = 0; index < count; index++) {
        winrt::com_ptr<IShellItem> item;
        PWSTR path = nullptr;

        if (SUCCEEDED(items->GetItemAt(index, item.put())) && SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
            paths.emplace_back(path);
            CoTaskMemFree(path);
        }
    }

    return paths;
}

}
