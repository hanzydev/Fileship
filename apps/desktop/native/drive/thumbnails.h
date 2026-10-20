#pragma once

#include <unknwn.h>
#include <winrt/base.h>

namespace drive {

extern CLSID const CLSID_FileshipThumbnails;
winrt::com_ptr<IClassFactory> thumbnailFactory();

}
