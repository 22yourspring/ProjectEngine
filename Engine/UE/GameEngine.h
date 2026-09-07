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
	void RenderWorld(FDynamicRHI& _DynamicRHI) override;
	void SetDefaultGameModeFactory(FGameModeFactory _Factory) override;
	void SetWorldInitializer(FWorldInitializer _Initializer) override;

	UWorld* GetWorld() const override { return __CurrentWorld.get(); }
	UGameInstance* GetGameInstance() const override { return __GameInstance.get(); }

private:
	std::unique_ptr<UGameInstance>	__GameInstance;
	std::unique_ptr<UWorld>			__CurrentWorld;
	FGameModeFactory				__DefaultGameModeFactory;
	FWorldInitializer				__WorldInitializer;
	mutable std::recursive_mutex	__WorldMutex;

	std::mutex			__PendingMapMutex;
	FName				__PendingMapName = NAME_None;
	bool				__bHasPendingMap = false;
	bool				__bStopPlayRequested = false;

	bool ApplyPendingMapChange();
};
