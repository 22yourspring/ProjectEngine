#include "pch.h"
#include "DynamicRHI.h"

#if defined(_WIN32)
#include "WindowsGDIRHI.h"
#endif

std::unique_ptr<FDynamicRHI> PlatformCreateDynamicRHI()
{
#if defined(_WIN32)
	return std::make_unique<FWindowsGDIRHI>();
#else
	return nullptr;
#endif
}
