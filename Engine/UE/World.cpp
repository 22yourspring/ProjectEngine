#include "pch.h"
#include "World.h"
#include "Scene.h"
#include <algorithm>

UWorld::UWorld()
	: __Scene(std::make_unique<FScene>()), __PersistentLevel(std::make_unique<ULevel>(this))
{
}

UWorld::~UWorld() = default;

void UWorld::Tick(float _DeltaTime)
{
	RunTickGroup(ETickingGroup::TG_PrePhysics, _DeltaTime);
}

void UWorld::RunTickGroup(ETickingGroup _TickGroup, float _DeltaTime)
{
	std::lock_guard<std::recursive_mutex> Lock(__WorldMutex);
	__bIsTicking = true;

	__TickTaskManager.RunTickGroup(_TickGroup, _DeltaTime);

	__bIsTicking = false;

	FlushPendingDestroyComponents();
	FlushPendingDestroyActors();
}

bool UWorld::DestroyActor(AActor* _Actor)
{
	std::lock_guard<std::recursive_mutex> Lock(__WorldMutex);

	if (nullptr == _Actor)
		return false;

	_Actor->MarkPendingDestroy();

	if (__bIsTicking)
	{
		auto Iter = std::find
		(
			__PendingDestroyActors.begin(),
			__PendingDestroyActors.end(),
			_Actor
		);

		if (__PendingDestroyActors.end() == Iter)
			__PendingDestroyActors.push_back(_Actor);

		return true;
	}

	UnregisterActorTickFunctions(_Actor);

	return __PersistentLevel->DestroyActor(_Actor);
}

void UWorld::RegisterComponentTickFunction(UActorComponent* _Component)
{
	std::lock_guard<std::recursive_mutex> Lock(__WorldMutex);

	if (nullptr == _Component)
		return;

	__TickTaskManager.AddTickFunction
	(
		&_Component->PrimaryComponentTick,
		_Component
	);
}

void UWorld::UnregisterComponentTickFunction(UActorComponent* _Component)
{
	std::lock_guard<std::recursive_mutex> Lock(__WorldMutex);

	if (nullptr == _Component)
		return;

	__TickTaskManager.RemoveTickFunction(&_Component->PrimaryComponentTick);
}

void UWorld::QueueComponentDestroy(AActor* _Owner, UActorComponent* _Component)
{
	std::lock_guard<std::recursive_mutex> Lock(__WorldMutex);

	if (nullptr == _Owner || nullptr == _Component)
		return;

	__PendingDestroyComponents.push_back({ _Owner, _Component });
}

void UWorld::RegisterActorTickFunctions(AActor* _Actor)
{
	std::lock_guard<std::recursive_mutex> Lock(__WorldMutex);

	if (nullptr == _Actor)
		return;

	__TickTaskManager.AddTickFunction(&_Actor->PrimaryActorTick, _Actor);

	for (const std::unique_ptr<UActorComponent>& Component : _Actor->GetComponents())
	{
		RegisterComponentTickFunction(Component.get());
		Component->RegisterComponentWithWorld(this);
	}
}

void UWorld::UnregisterActorTickFunctions(AActor* _Actor)
{
	std::lock_guard<std::recursive_mutex> Lock(__WorldMutex);

	if (nullptr == _Actor)
		return;

	__TickTaskManager.RemoveTickFunction(&_Actor->PrimaryActorTick);

	for (const std::unique_ptr<UActorComponent>& Component : _Actor->GetComponents())
	{
		Component->UnregisterComponent();
		UnregisterComponentTickFunction(Component.get());
	}
}

void UWorld::FlushPendingDestroyActors()
{
	std::vector<AActor*> PendingActors = std::move(__PendingDestroyActors);
	__PendingDestroyActors.clear();

	for (AActor* Actor : PendingActors)
	{
		if (nullptr == Actor)
			continue;

		UnregisterActorTickFunctions(Actor);
		__PersistentLevel->DestroyActor(Actor);
	}
}

void UWorld::FlushPendingDestroyComponents()
{
	std::vector<FPendingComponent> PendingComponents =
		std::move(__PendingDestroyComponents);

	__PendingDestroyComponents.clear();

	for (const FPendingComponent& Entry : PendingComponents)
	{
		if (nullptr == Entry.Owner || nullptr == Entry.Component)
			continue;

		Entry.Owner->DestroyComponent(Entry.Component);
	}
}
