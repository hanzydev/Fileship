#pragma once

#include <unknwn.h>
#include <winrt/base.h>

#include <string>

namespace harbor {

extern CLSID const CLSID_FileshipStatus;
winrt::com_ptr<IClassFactory> statusFactory();
void setHarborState(std::wstring const& state);

}
