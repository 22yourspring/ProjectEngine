#pragma once

#include "Object.h"
#include "Level.h"
#include "TickTaskManager.h"
#include <mutex>
#include <type_traits>

class FScene;

UCLASS(MinimalAPI)
class UWorld : public UObject
{
	GENERATED_BODY()

	friend class AActor;
	friend class UActorComponent;

public:
	UWorld();
	~UWorld();

	template<typename T, typename... Args>
	T* SpawnActor(Args&&... _Args);

	bool DestroyActor(AActor* _Actor);
	void Tick(float _DeltaTime);
	void RunTickGroup(ETickingGroup _TickGroup, float _DeltaTime);

	bool IsTicking() const { return __bIsTicking; }

	ULevel* GetPersistentLevel() const { return __PersistentLevel.get(); }
	FScene* GetScene() const { return __Scene.get(); }

	void QueueComponentDestroy(AActor* _Owner, UActorComponent* _Component);

private:
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

	std::unique_ptr<FScene>				__Scene;
	std::unique_ptr<ULevel>				__PersistentLevel;
	FTickTaskManager					__TickTaskManager;
	std::vector<AActor*>				__PendingDestroyActors;
	std::vector<FPendingComponent>		__PendingDestroyComponents;
	std::recursive_mutex				__WorldMutex;
	bool								__bIsTicking = false;
};

#include "World.inl"
