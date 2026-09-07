#include "pch.h"
#include "ProjectGameInstanceSubsystem.h"

HRESULT UProjectGameInstanceSubsystem::Initialize()
{
	__bProjectInitialized = false;
	return S_OK;
}

void UProjectGameInstanceSubsystem::Deinitialize()
{
	__bProjectInitialized = false;
}
