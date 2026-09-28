#include "pch.h"
#include "GameEngine.h"
#include "World.h"
#include "Scene.h"
#include "DynamicRHI.h"
#include "WorldPersistence.h"
#include "ResourceEngineSystem.h"
#include "PathEngineSystem.h"
#include "GameMapsSettings.h"
#include "ModuleManager.h"
#include "ModuleInterface.h"
#include <fstream>

UGameEngine::UGameEngine() = default;
UGameEngine::~UGameEngine() = default;

void UGameEngine::WithWorld(const std::function<void(UWorld*)>& _Action)
{
    std::lock_guard<std::recursive_mutex> Lock(__WorldMutex);
    _Action(__CurrentWorld.get());
}

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
	std::lock_guard<std::recursive_mutex> Lock(__WorldMutex);
	UEngine::Tick(_DeltaTime);

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

bool UGameEngine::LoadEditorMap(const TCHAR* _LevelName)
{
    return SetEditorWorld(std::make_unique<UWorld>(FName(_LevelName)), _LevelName);
}

bool UGameEngine::SetEditorWorld(std::unique_ptr<UWorld> _World, const TCHAR* _LevelName)
{
    if (!_World) return false;
    if (!_LevelName || !*_LevelName) return false;
    std::lock_guard<std::recursive_mutex> WorldLock(__WorldMutex);
    std::lock_guard<std::mutex> PendingLock(__PendingMapMutex);
    FName Name(_LevelName);
    __EditorMapName = Name;
    _World->SetDebugPauseExecution(true);
    __CurrentWorld = std::move(_World);
    __bHasPendingMap = false;
    __bStopPlayRequested = false;
    ++__WorldGeneration;
    return true;
}

bool UGameEngine::PlayEditorMap()
{
    std::lock_guard<std::recursive_mutex> Lock(__WorldMutex);
    FString Error;
    if (!FWorldPersistence::Capture(__CurrentWorld.get(), __EditorSnapshot, Error)) return false;
    return __EditorMapName.IsValid() && LoadMap(__EditorMapName);
}

bool UGameEngine::RestoreEditorSession()
{
    auto* Resources = GetEngineSystem<ResourceEngineSystem>();
    if (!Resources || __EditorSnapshot.empty()) return false;
    FString Error;
    auto World = FWorldPersistence::Restore(__EditorSnapshot, __EditorMapName.ToString(), Resources->GetPackageStore(), Error);
    return World && SetEditorWorld(std::move(World), *__EditorMapName.ToString());
}
void UGameEngine::RenameEditorMap(const TCHAR* _Name)
{
    std::lock_guard<std::recursive_mutex> Lock(__WorldMutex);
    __EditorMapName = FName(_Name);
    if (__CurrentWorld) __CurrentWorld->GetPersistentLevel()->SetLevelName(__EditorMapName);
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
        bStopPlay ? (__EditorMapName.IsValid() ? std::make_unique<UWorld>(__EditorMapName) : std::make_unique<UWorld>())
            : std::make_unique<UWorld>(LevelName);
	if (!NewWorld)
		return false;

    int MapMode = -1;
    if (!bStopPlay && (__EditorSnapshot.empty() || LevelName != __EditorMapName))
    {
        auto* Paths = GetEngineSystem<PathEngineSystem>();
        auto* Resources = GetEngineSystem<ResourceEngineSystem>();
        auto MapName = LevelName.ToString();
        if (MapName.ToWide().rfind(L"/Game/", 0) != 0) MapName = FString(TEXT("/Game/Levels/")) + MapName;
        const auto File = UGameMapsSettings::ResolveMap(Paths->GetProjectContentDirectory(), MapName);
        std::ifstream Header(File, std::ios::binary); char Magic[7] = {}; Header.read(Magic, 7);
        if (std::memcmp(Magic, "PEWORLD", 7) == 0)
        {
            std::vector<uint8> Data; FString Error;
            if (!FWorldPersistence::Read(File, Data, MapMode, Error)) return false;
            NewWorld = FWorldPersistence::Restore(Data, LevelName.ToString(), Resources->GetPackageStore(), Error);
            if (!NewWorld) return false;
        }
        if (MapMode < 0)
        {
            UGameMapsSettings Settings; Settings.Load(Paths->GetProjectDirectory());
            MapMode = Settings.__UseEngineGameMode ? 0 : 1;
        }
    }

    if (!__EditorSnapshot.empty() && (bStopPlay || LevelName == __EditorMapName))
    {
        auto* Resources = GetEngineSystem<ResourceEngineSystem>();
        FString Error;
        NewWorld = FWorldPersistence::Restore(__EditorSnapshot, __EditorMapName.ToString(), Resources->GetPackageStore(), Error);
        if (!NewWorld) return false;
    }

	if (!bStopPlay && __DefaultGameModeFactory)
	{
        auto* Module = FModuleManager::GetGameModule();
        NewWorld->SetGameMode(MapMode >= 0 && Module ? Module->CreateGameMode(MapMode == 0) : __DefaultGameModeFactory());
		if (false == NewWorld->StartPlay())
			return false;
	}

	if (!bStopPlay && __WorldInitializer && false == __WorldInitializer(NewWorld.get()))
		return false;

    if (bStopPlay) NewWorld->SetDebugPauseExecution(true);
	__CurrentWorld = std::move(NewWorld);
    ++__WorldGeneration;
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
