#pragma once

#include "Object.h"
#include "Level.h"
#include "LevelStreaming.h"
#include "GameModeBase.h"
#include "TickTaskManager.h"
#include "PhysScene_Chaos.h"
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
    friend class FPhysScene_Chaos;

public:
	explicit UWorld(const FName& _PersistentLevelName = FName(TEXT("PersistentLevel")));
	~UWorld();
    UWorld* GetWorld() const override { return const_cast<UWorld*>(this); }

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
	FPhysScene* GetPhysicsScene() const { return __PhysicsScene.get(); }
	bool LineTraceSingleByChannel(FHitResult& _OutHit, const FVector& _Start, const FVector& _End, ECollisionChannel _TraceChannel, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& _ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
	bool SweepSingleByChannel(FHitResult& _OutHit, const FVector& _Start, const FVector& _End, const FQuat& _Rot, ECollisionChannel _TraceChannel, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& _ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
	bool OverlapMultiByChannel(std::vector<FOverlapResult>& _OutOverlaps, const FVector& _Pos, const FQuat& _Rot, ECollisionChannel _TraceChannel, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& _ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;

    bool LineTraceMultiByChannel(std::vector<FHitResult>& _OutHits, const FVector& _Start, const FVector& _End, ECollisionChannel _TraceChannel, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& _ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
    bool LineTraceTestByChannel(const FVector& _Start, const FVector& _End, ECollisionChannel _TraceChannel, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& _ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
    bool LineTraceSingleByObjectType(FHitResult& _OutHit, const FVector& _Start, const FVector& _End, const FCollisionObjectQueryParams& _ObjectQueryParams, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam) const;
    bool LineTraceMultiByObjectType(std::vector<FHitResult>& _OutHits, const FVector& _Start, const FVector& _End, const FCollisionObjectQueryParams& _ObjectQueryParams, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam) const;
    bool LineTraceTestByObjectType(const FVector& _Start, const FVector& _End, const FCollisionObjectQueryParams& _ObjectQueryParams, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam) const;
    bool LineTraceSingleByProfile(FHitResult& _OutHit, const FVector& _Start, const FVector& _End, FName _ProfileName, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam) const;
    bool LineTraceMultiByProfile(std::vector<FHitResult>& _OutHits, const FVector& _Start, const FVector& _End, FName _ProfileName, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam) const;
    bool LineTraceTestByProfile(const FVector& _Start, const FVector& _End, FName _ProfileName, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam) const;
    bool SweepMultiByChannel(std::vector<FHitResult>& _OutHits, const FVector& _Start, const FVector& _End, const FQuat& _Rot, ECollisionChannel _TraceChannel, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& _ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
    bool SweepTestByChannel(const FVector& _Start, const FVector& _End, const FQuat& _Rot, ECollisionChannel _TraceChannel, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& _ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
    bool SweepSingleByObjectType(FHitResult& _OutHit, const FVector& _Start, const FVector& _End, const FQuat& _Rot, const FCollisionObjectQueryParams& _ObjectQueryParams, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam) const;
    bool SweepMultiByObjectType(std::vector<FHitResult>& _OutHits, const FVector& _Start, const FVector& _End, const FQuat& _Rot, const FCollisionObjectQueryParams& _ObjectQueryParams, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam) const;
    bool SweepTestByObjectType(const FVector& _Start, const FVector& _End, const FQuat& _Rot, const FCollisionObjectQueryParams& _ObjectQueryParams, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam) const;
    bool SweepSingleByProfile(FHitResult& _OutHit, const FVector& _Start, const FVector& _End, const FQuat& _Rot, FName _ProfileName, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam) const;
    bool SweepMultiByProfile(std::vector<FHitResult>& _OutHits, const FVector& _Start, const FVector& _End, const FQuat& _Rot, FName _ProfileName, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam) const;
    bool SweepTestByProfile(const FVector& _Start, const FVector& _End, const FQuat& _Rot, FName _ProfileName, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam) const;
    bool OverlapAnyTestByChannel(const FVector& _Pos, const FQuat& _Rot, ECollisionChannel _TraceChannel, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& _ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
    bool OverlapBlockingTestByChannel(const FVector& _Pos, const FQuat& _Rot, ECollisionChannel _TraceChannel, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& _ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
    bool OverlapMultiByObjectType(std::vector<FOverlapResult>& _OutOverlaps, const FVector& _Pos, const FQuat& _Rot, const FCollisionObjectQueryParams& _ObjectQueryParams, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam) const;
    bool OverlapAnyTestByObjectType(const FVector& _Pos, const FQuat& _Rot, const FCollisionObjectQueryParams& _ObjectQueryParams, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam) const;
    bool OverlapMultiByProfile(std::vector<FOverlapResult>& _OutOverlaps, const FVector& _Pos, const FQuat& _Rot, FName _ProfileName, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam) const;
    bool OverlapAnyTestByProfile(const FVector& _Pos, const FQuat& _Rot, FName _ProfileName, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam) const;
    bool OverlapBlockingTestByProfile(const FVector& _Pos, const FQuat& _Rot, FName _ProfileName, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params = FCollisionQueryParams::DefaultQueryParam) const;

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

	std::unique_ptr<FPhysScene> __PhysicsScene;
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
