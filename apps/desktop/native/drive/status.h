#pragma once

#include <unknwn.h>
#include <winrt/base.h>

#include <string>

namespace drive {

extern CLSID const CLSID_FileshipStatus;
winrt::com_ptr<IClassFactory> statusFactory();
void setDriveState(std::wstring const& state);

}
