#include "pch.h"
#include "GameInstance.h"

UGameInstance::UGameInstance() = default;

UGameInstance::~UGameInstance()
{
	Shutdown();
}

HRESULT UGameInstance::Initialize()
{
	std::vector<UGameInstanceSubsystem*> Subsystems;

	{
		std::lock_guard<std::mutex> Lock(__SubsystemMutex);
		for (auto& Pair : __Subsystems)
			Subsystems.push_back(Pair.second.get());
	}

	for (UGameInstanceSubsystem* Subsystem : Subsystems)
	{
		if (Subsystem && FAILED(Subsystem->Initialize()))
			return E_FAIL;
	}

	__bInitialized = true;

	return S_OK;
}

void UGameInstance::Shutdown()
{
	__bInitialized = false;
	std::vector<std::unique_ptr<UGameInstanceSubsystem>> Subsystems;

	{
		std::lock_guard<std::mutex> Lock(__SubsystemMutex);
		for (auto& Pair : __Subsystems)
			Subsystems.push_back(std::move(Pair.second));
		__Subsystems.clear();
	}

	for (std::unique_ptr<UGameInstanceSubsystem>& Subsystem : Subsystems)
	{
		if (Subsystem)
			Subsystem->Deinitialize();
	}
}
