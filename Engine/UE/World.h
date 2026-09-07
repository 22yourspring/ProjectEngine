#pragma once

#include "Object.h"
#include "Level.h"
#include "LevelStreaming.h"
#include "GameModeBase.h"
#include "TickTaskManager.h"
#include <atomic>
#include <mutex>
#include <type_traits>
#include <vector>

class FScene;

UCLASS(MinimalAPI)
class ENGINE_API UWorld : public UObject
{
	GENERATED_BODY()

	friend class AActor;
	friend class UActorComponent;

public:
	explicit UWorld(const FName& _PersistentLevelName = FName(TEXT("PersistentLevel")));
	~UWorld();

	template<typename T, typename... Args>
	T* SpawnActor(Args&&... _Args);

	bool DestroyActor(AActor* _Actor);
	void Tick(float _DeltaTime);
	void RunTickGroup(ETickingGroup _TickGroup, float _DeltaTime);
	bool StartPlay();

	template<typename T = AGameModeBase>
	T* GetAuthGameMode() const;
	void RegisterEngineSystemTickFunction(IEngineSystem* _EngineSystem);
	void UnregisterEngineSystemTickFunction(IEngineSystem* _EngineSystem);

	bool IsTicking() const { return __bIsTicking; }
	void SetDebugPauseExecution(bool _bPaused) { __bDebugPauseExecution.store(_bPaused); }
	bool IsDebugPauseExecution() const { return __bDebugPauseExecution.load(); }
	void UpdateLevelStreaming();

	ULevel* GetPersistentLevel() const { return __PersistentLevel.get(); }
	bool IsPersistentLevel(const TCHAR* _LevelName) const;
	const std::vector<std::unique_ptr<ULevelStreaming>>& GetStreamingLevels() const
	{
		return __StreamingLevels;
	}
	std::vector<ULevel*> GetLevels() const;
	FScene* GetScene() const { return __Scene.get(); }

	bool AddStreamingLevel(std::unique_ptr<ULevelStreaming> _StreamingLevel);
	bool RemoveStreamingLevel(ULevelStreaming* _StreamingLevel);

	void QueueComponentDestroy(AActor* _Owner, UActorComponent* _Component);

private:
	friend class UGameEngine;
	void SetGameMode(std::unique_ptr<AGameModeBase> _GameMode);
	AGameModeBase* GetGameMode() const { return __GameMode.get(); }
	struct FPendingComponent
	{
		AActor*				Owner = nullptr;
		UActorComponent*	Component = nullptr;
	};

	void RegisterActorTickFunctions(AActor* _Actor);
	void UnregisterActorTickFunctions(AActor* _Actor);
	void RegisterComponentTickFunction(UActorComponent* _Component);
	void UnregisterComponentTickFunction(UActorComponent* _Component);
	void FlushPendingDestroyActors();
	void FlushPendingDestroyComponents();

	std::unique_ptr<FScene>							__Scene;
	std::unique_ptr<AGameModeBase>					__GameMode;
	std::unique_ptr<ULevel>							__PersistentLevel;
	std::vector<std::unique_ptr<ULevelStreaming>>	__StreamingLevels;
	FTickTaskManager								__TickTaskManager;
	std::vector<AActor*>							__PendingDestroyActors;
	std::vector<FPendingComponent>					__PendingDestroyComponents;
	std::recursive_mutex							__WorldMutex;
	std::atomic_bool							__bDebugPauseExecution = false;
	bool											__bIsTicking = false;
};

#include "World.inl"
