#pragma once

#include <unknwn.h>
#include <winrt/base.h>
#include <winrt/Windows.Data.Json.h>

namespace harbor {

extern CLSID const CLSID_FileshipStatus;
winrt::com_ptr<IClassFactory> statusFactory();
void setHarborStatus(winrt::Windows::Data::Json::JsonObject const& status);

}
