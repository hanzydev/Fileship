#include <host/app.h>

#include <windows.h>
#include <objbase.h>
#include <shobjidl.h>
#include <winrt/base.h>

namespace fileship::host {

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
