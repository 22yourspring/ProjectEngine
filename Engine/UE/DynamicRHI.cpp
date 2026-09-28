#include "pch.h"
#include "DynamicRHI.h"

#include <algorithm>
#include <cwctype>

#if defined(_WIN32)
#include "D3D11DynamicRHI.h"
#include "Windows/WindowsGDIRHI.h"
#endif

namespace
{
	ERHIInterfaceType GPreferredRHIInterface = ERHIInterfaceType::GDI;

	bool HasCommandLineSwitch(const wchar_t* _Switch)
	{
		if (nullptr == _Switch)
			return false;

		FString CommandLine = GetCommandLineW();
		std::transform(CommandLine.begin(), CommandLine.end(), CommandLine.begin(),
			[](wchar_t Character) { return static_cast<wchar_t>(std::towlower(Character)); });
		return CommandLine.Contains(_Switch, ESearchCase::CaseSensitive);
	}
}

void RHISetPreferredInterface(ERHIInterfaceType _InterfaceType)
{
	GPreferredRHIInterface = _InterfaceType;
}

std::unique_ptr<FDynamicRHI> PlatformCreateDynamicRHI()
{
#if defined(_WIN32)
	std::unique_ptr<IDynamicRHIModule> DynamicRHIModule;
	const bool bForceGDI = HasCommandLineSwitch(TEXT("-gdi"));
	const bool bUseD3D11 = false == bForceGDI &&
		(HasCommandLineSwitch(TEXT("-d3d11")) ||
		 ERHIInterfaceType::D3D11 == GPreferredRHIInterface);
	if (bUseD3D11)
		DynamicRHIModule = std::make_unique<FD3D11DynamicRHIModule>();
	else
		DynamicRHIModule = std::make_unique<FWindowsGDIRHIModule>();

	if (nullptr == DynamicRHIModule || false == DynamicRHIModule->IsSupported())
		return nullptr;

	return DynamicRHIModule->CreateRHI();
#else
	return nullptr;
#endif
}
