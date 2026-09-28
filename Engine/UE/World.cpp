#include "pch.h"
#include "World.h"
#include "Scene.h"
#include "GameModeBase.h"
#include "EngineSystem.h"
#include <algorithm>

UWorld::UWorld(const FName& _PersistentLevelName)
	: __PhysicsScene(std::make_unique<FPhysScene>()), __Scene(std::make_unique<FScene>()),
	  __PersistentLevel(std::make_unique<ULevel>(this, _PersistentLevelName))
{
}

UWorld::~UWorld() = default;

bool UWorld::LineTraceSingleByChannel(FHitResult& _OutHit, const FVector& _Start, const FVector& _End, ECollisionChannel _TraceChannel, const FCollisionQueryParams& _Params, const FCollisionResponseParams& _ResponseParam) const
{
    return __PhysicsScene->LineTraceSingleByChannel(_OutHit, _Start, _End, _TraceChannel, _Params, _ResponseParam);
}
bool UWorld::SweepSingleByChannel(FHitResult& _OutHit, const FVector& _Start, const FVector& _End, const FQuat& _Rot, ECollisionChannel _TraceChannel, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params, const FCollisionResponseParams& _ResponseParam) const
{
    return __PhysicsScene->SweepSingleByChannel(_OutHit, _Start, _End, _Rot, _TraceChannel, _CollisionShape, _Params, _ResponseParam);
}
bool UWorld::OverlapMultiByChannel(std::vector<FOverlapResult>& _OutOverlaps, const FVector& _Pos, const FQuat& _Rot, ECollisionChannel _TraceChannel, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params, const FCollisionResponseParams& _ResponseParam) const
{
    return __PhysicsScene->OverlapMultiByChannel(_OutOverlaps, _Pos, _Rot, _TraceChannel, _CollisionShape, _Params, _ResponseParam);
}

bool UWorld::IsPersistentLevel(const TCHAR* _LevelName) const
{
	return nullptr != _LevelName && __PersistentLevel &&
		__PersistentLevel->GetLevelName() == FName(_LevelName);
}

void UWorld::SetGameMode(std::unique_ptr<AGameModeBase> _GameMode)
{
	__GameMode = std::move(_GameMode);
}

bool UWorld::StartPlay()
{
	return nullptr != __GameMode && __GameMode->StartPlay(this);
}

std::vector<ULevel*> UWorld::GetLevels() const
{
	std::vector<ULevel*> Levels;
	Levels.reserve(1 + __StreamingLevels.size());

	if (__PersistentLevel)
		Levels.push_back(__PersistentLevel.get());

	for (const std::unique_ptr<ULevelStreaming>& StreamingLevel : __StreamingLevels)
	{
		if (StreamingLevel && StreamingLevel->GetLoadedLevel())
			Levels.push_back(StreamingLevel->GetLoadedLevel());
	}

	return Levels;
}

bool UWorld::AddStreamingLevel(std::unique_ptr<ULevelStreaming> _StreamingLevel)
{
	if (!_StreamingLevel)
		return false;

	__StreamingLevels.push_back(std::move(_StreamingLevel));
	return true;
}

bool UWorld::RemoveStreamingLevel(ULevelStreaming* _StreamingLevel)
{
	if (!_StreamingLevel)
		return false;

	auto Iter = std::find_if(
		__StreamingLevels.begin(),
		__StreamingLevels.end(),
		[_StreamingLevel](const std::unique_ptr<ULevelStreaming>& _Entry)
		{
			return _Entry.get() == _StreamingLevel;
		});

	if (__StreamingLevels.end() == Iter)
		return false;

	(*Iter)->UnloadLevel();
	__StreamingLevels.erase(Iter);
	return true;
}

void UWorld::Tick(float _DeltaTime)
{
    if (IsDebugPauseExecution()) return;
	UpdateLevelStreaming();

	if (__PersistentLevel)
		__PersistentLevel->Tick(_DeltaTime);

	for (const std::unique_ptr<ULevelStreaming>& StreamingLevel : __StreamingLevels)
	{
		if (StreamingLevel && StreamingLevel->GetLoadedLevel())
			StreamingLevel->GetLoadedLevel()->Tick(_DeltaTime);
	}

	RunTickGroup(ETickingGroup::TG_PrePhysics, _DeltaTime);
	RunTickGroup(ETickingGroup::TG_StartPhysics, _DeltaTime);
	RunTickGroup(ETickingGroup::TG_DuringPhysics, _DeltaTime);
	RunTickGroup(ETickingGroup::TG_EndPhysics, _DeltaTime);
	RunTickGroup(ETickingGroup::TG_PostPhysics, _DeltaTime);
}

void UWorld::UpdateLevelStreaming()
{
	for (const std::unique_ptr<ULevelStreaming>& StreamingLevel : __StreamingLevels)
	{
		if (!StreamingLevel)
			continue;

		if (StreamingLevel->ShouldBeLoaded())
		{
			if (!StreamingLevel->HasLoadedLevel())
			{
				StreamingLevel->SetLoadedLevel(std::make_unique<ULevel>(
					this,
					StreamingLevel->GetWorldAssetPackageFName()));
			}

			StreamingLevel->GetLoadedLevel()->SetVisible(
				StreamingLevel->ShouldBeVisible());
		}
		else if (StreamingLevel->HasLoadedLevel())
		{
			StreamingLevel->UnloadLevel();
		}
	}
}

void UWorld::RunTickGroup(ETickingGroup _TickGroup, float _DeltaTime)
{
	std::lock_guard<std::recursive_mutex> Lock(__WorldMutex);
    if (IsDebugPauseExecution()) return;
	__bIsTicking = true;
    if (_TickGroup == ETickingGroup::TG_StartPhysics)
    {
        __PhysicsScene->SetUpForFrame(nullptr, _DeltaTime, 0, .25f, 1.f / 120.f, 32, true);
        __PhysicsScene->StartFrame();
    }
    if (_TickGroup == ETickingGroup::TG_EndPhysics) __PhysicsScene->EndFrame();

	__TickTaskManager.RunTickGroup(_TickGroup, _DeltaTime);

	for (const std::unique_ptr<ULevelStreaming>& StreamingLevel : __StreamingLevels)
	{
		if (StreamingLevel && StreamingLevel->GetLoadedLevel())
			StreamingLevel->GetLoadedLevel()->RunTickGroup(_TickGroup, _DeltaTime);
	}

	__bIsTicking = false;

	FlushPendingDestroyComponents();
	FlushPendingDestroyActors();
}

void UWorld::RegisterEngineSystemTickFunction(IEngineSystem* _EngineSystem)
{
	std::lock_guard<std::recursive_mutex> Lock(__WorldMutex);

	if (nullptr == _EngineSystem || false == _EngineSystem->UsesTickGroup())
		return;

	__TickTaskManager.AddTickFunction(
		&_EngineSystem->PrimaryEngineSystemTick, _EngineSystem);
}

void UWorld::UnregisterEngineSystemTickFunction(IEngineSystem* _EngineSystem)
{
	std::lock_guard<std::recursive_mutex> Lock(__WorldMutex);

	if (nullptr == _EngineSystem || false == _EngineSystem->UsesTickGroup())
		return;

	__TickTaskManager.RemoveTickFunction(
		&_EngineSystem->PrimaryEngineSystemTick);
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

	for (UActorComponent* Component : _Actor->GetComponents())
	{
		RegisterComponentTickFunction(Component);
		Component->RegisterComponentWithWorld(this);
	}
}

void UWorld::UnregisterActorTickFunctions(AActor* _Actor)
{
	std::lock_guard<std::recursive_mutex> Lock(__WorldMutex);

	if (nullptr == _Actor)
		return;

	__TickTaskManager.RemoveTickFunction(&_Actor->PrimaryActorTick);

	for (UActorComponent* Component : _Actor->GetComponents())
	{
		Component->UnregisterComponent();
		UnregisterComponentTickFunction(Component);
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
