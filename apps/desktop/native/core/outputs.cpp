#include "common.h"

#include <dxgi1_2.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace native {

namespace {

Napi::Value listOutputs(Napi::CallbackInfo const& info) {
    auto env = info.Env();
    auto result = Napi::Array::New(env);

    ComPtr<IDXGIFactory1> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return result;

    uint32_t count = 0;
    ComPtr<IDXGIAdapter1> adapter;

    for (UINT adapterIndex = 0; factory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND;
         adapterIndex++) {
        ComPtr<IDXGIOutput> output;

        for (UINT outputIndex = 0; adapter->EnumOutputs(outputIndex, &output) != DXGI_ERROR_NOT_FOUND;
             outputIndex++) {
            DXGI_OUTPUT_DESC desc{};

            if (SUCCEEDED(output->GetDesc(&desc))) {
                auto const& coordinates = desc.DesktopCoordinates;
                auto item = Napi::Object::New(env);

                item.Set("adapter", adapterIndex);
                item.Set("output", outputIndex);
                item.Set("rect", rectObject(env, Rect{coordinates.left, coordinates.top,
                                                      coordinates.right - coordinates.left,
                                                      coordinates.bottom - coordinates.top}));
                result.Set(count++, item);
            }

            output.Reset();
        }

        adapter.Reset();
    }

    return result;
}

}

void registerOutputs(Napi::Env env, Napi::Object exports) {
    exports.Set("listOutputs", Napi::Function::New(env, listOutputs));
}

}
