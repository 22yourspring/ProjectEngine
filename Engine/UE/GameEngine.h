#pragma once

#include "Engine.h"
#include "GameInstance.h"
#include <mutex>

class UWorld;

UCLASS(MinimalAPI)
class ENGINE_API UGameEngine : public UEngine
{
	GENERATED_BODY()

public:
	UGameEngine();
	~UGameEngine() override;

	HRESULT Initialize() override;
	void Deinitialize() override;
	void Tick(float _DeltaTime) override;
	void RunTickGroup(ETickingGroup _TickGroup, float _DeltaTime) override;
	bool LoadMap(const FName& _LevelName) override;
	bool StopPlay() override;
    bool LoadEditorMap(const TCHAR* _LevelName);
    bool SetEditorWorld(std::unique_ptr<UWorld> _World, const TCHAR* _LevelName);
    bool PlayEditorMap();
    bool RestoreEditorSession();
    void RenameEditorMap(const TCHAR* _Name);
	void RenderWorld(FDynamicRHI& _DynamicRHI) override;
	void SetDefaultGameModeFactory(FGameModeFactory _Factory) override;
	void SetWorldInitializer(FWorldInitializer _Initializer) override;
    void WithWorld(const std::function<void(UWorld*)>& _Action);
    uint64 GetWorldGeneration() const { return __WorldGeneration; }

	UWorld* GetWorld() const override { return __CurrentWorld.get(); }
	UGameInstance* GetGameInstance() const override { return __GameInstance.get(); }

private:
	std::unique_ptr<UGameInstance>	__GameInstance;
	std::unique_ptr<UWorld>			__CurrentWorld;
	FGameModeFactory				__DefaultGameModeFactory;
	FWorldInitializer				__WorldInitializer;
	mutable std::recursive_mutex	__WorldMutex;
    uint64 __WorldGeneration = 1;

	std::mutex			__PendingMapMutex;
	FName				__PendingMapName = NAME_None;
	bool				__bHasPendingMap = false;
	bool				__bStopPlayRequested = false;
    FName __EditorMapName;
    std::vector<uint8> __EditorSnapshot;

	bool ApplyPendingMapChange();
};
