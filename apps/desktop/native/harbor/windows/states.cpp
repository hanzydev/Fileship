#include "common.h"

#include <winrt/Windows.Storage.Provider.h>
#include <winrt/Windows.Storage.h>

using namespace winrt;
using namespace Windows::Storage;
using namespace Windows::Storage::Provider;

namespace harbor {

namespace {

wchar_t const* iconOf(int32_t id) {
    if (id == 1) return L"imageres.dll,-59";
    if (id == 2) return L"shell32.dll,-16752";
    if (id == 3) return L"imageres.dll,-107";
    return L"imageres.dll,-5409";
}

}

JsonArray setStates(JsonArray const& items) {
    JsonArray failed;

    for (auto const& value : items) {
        auto item = value.GetObject();
        std::wstring path(item.GetNamedString(L"path"));

        try {
            std::vector<StorageProviderItemProperty> properties;

            for (auto const& entry : item.GetNamedArray(L"properties")) {
                auto object = entry.GetObject();
                auto id = static_cast<int32_t>(object.GetNamedNumber(L"id"));

                StorageProviderItemProperty property;
                property.Id(id);
                property.Value(object.GetNamedString(L"value"));
                property.IconResource(iconOf(id));
                properties.push_back(property);
            }

            auto file = StorageFile::GetFileFromPathAsync(fullPath(path)).get();
            StorageProviderItemProperties::SetAsync(file, single_threaded_vector(std::move(properties))).get();
        } catch (hresult_error const& error) {
            log(L"Could not set the states of " + path + L": " + std::wstring(error.message()));
            failed.Append(JsonValue::CreateStringValue(path));
        }
    }

    return failed;
}

}
