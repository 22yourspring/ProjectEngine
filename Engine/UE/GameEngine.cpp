#include "pch.h"
#include "GameEngine.h"
#include "World.h"
#include "Scene.h"
#include "DynamicRHI.h"

UGameEngine::UGameEngine() = default;
UGameEngine::~UGameEngine() = default;

HRESULT UGameEngine::Initialize()
{
	if (nullptr == __GameInstance)
		__GameInstance = std::make_unique<UGameInstance>();

	if (FAILED(__GameInstance->Initialize()))
		return E_FAIL;

	if (nullptr == __CurrentWorld)
		__CurrentWorld = std::make_unique<UWorld>();

	if (__DefaultGameModeFactory)
	{
		__CurrentWorld->SetGameMode(__DefaultGameModeFactory());
		if (false == __CurrentWorld->StartPlay())
			return E_FAIL;
	}

	if (FAILED(UEngine::Initialize()))
	{
		__CurrentWorld.reset();
		__GameInstance->Shutdown();
		__GameInstance.reset();
		return E_FAIL;
	}

	return S_OK;
}

void UGameEngine::Deinitialize()
{
	__CurrentWorld.reset();

	if (__GameInstance)
	{
		__GameInstance->Shutdown();
		__GameInstance.reset();
	}

	UEngine::Deinitialize();
}

void UGameEngine::Tick(float _DeltaTime)
{
	UEngine::Tick(_DeltaTime);

	std::lock_guard<std::recursive_mutex> Lock(__WorldMutex);
	if (__CurrentWorld && false == __CurrentWorld->IsDebugPauseExecution())
		__CurrentWorld->Tick(_DeltaTime);

	ApplyPendingMapChange();
}

void UGameEngine::RunTickGroup(ETickingGroup _TickGroup, float _DeltaTime)
{
	std::lock_guard<std::recursive_mutex> Lock(__WorldMutex);
	if (__CurrentWorld && false == __CurrentWorld->IsDebugPauseExecution())
		__CurrentWorld->RunTickGroup(_TickGroup, _DeltaTime);
}

bool UGameEngine::LoadMap(const FName& _LevelName)
{
	if (!_LevelName.IsValid())
		return false;

	std::lock_guard<std::mutex> Lock(__PendingMapMutex);
	__PendingMapName = _LevelName;
	__bHasPendingMap = true;
	__bStopPlayRequested = false;
	return true;
}

bool UGameEngine::StopPlay()
{
	std::lock_guard<std::mutex> Lock(__PendingMapMutex);
	__PendingMapName = NAME_None;
	__bHasPendingMap = true;
	__bStopPlayRequested = true;
	return true;
}

bool UGameEngine::ApplyPendingMapChange()
{
	FName LevelName;
	bool bStopPlay = false;
	{
		std::lock_guard<std::mutex> Lock(__PendingMapMutex);
		if (false == __bHasPendingMap)
			return true;
		LevelName = __PendingMapName;
		bStopPlay = __bStopPlayRequested;
		__PendingMapName = NAME_None;
		__bHasPendingMap = false;
		__bStopPlayRequested = false;
	}

	std::unique_ptr<UWorld> NewWorld =
		bStopPlay ? std::make_unique<UWorld>() : std::make_unique<UWorld>(LevelName);
	if (!NewWorld)
		return false;

	if (!bStopPlay && __DefaultGameModeFactory)
	{
		NewWorld->SetGameMode(__DefaultGameModeFactory());
		if (false == NewWorld->StartPlay())
			return false;
	}

	if (!bStopPlay && __WorldInitializer && false == __WorldInitializer(NewWorld.get()))
		return false;

	__CurrentWorld = std::move(NewWorld);
	return true;
}

void UGameEngine::RenderWorld(FDynamicRHI& _DynamicRHI)
{
	std::lock_guard<std::recursive_mutex> Lock(__WorldMutex);
	if (__CurrentWorld && __CurrentWorld->GetScene())
		__CurrentWorld->GetScene()->Render(_DynamicRHI);
}

void UGameEngine::SetDefaultGameModeFactory(FGameModeFactory _Factory)
{
	__DefaultGameModeFactory = std::move(_Factory);

	
	
}

void UGameEngine::SetWorldInitializer(FWorldInitializer _Initializer)
{
	__WorldInitializer = std::move(_Initializer);
}
