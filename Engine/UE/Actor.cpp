#include "pch.h"
#include "Actor.h"
#include "Level.h"
#include "World.h"
#include "SceneComponent.h"
#include <algorithm>

void FActorTickFunction::ExecuteTick(AActor* _Target, float _DeltaTime)
{
	if (nullptr == _Target || false == bCanEverTick || _Target->IsPendingDestroy())
		return;

	_Target->TickActor(_DeltaTime);
}

void AActor::TickActor(float _DeltaTime)
{
	Tick(_DeltaTime);
}

UWorld* AActor::GetWorld() const
{
	return __Level ? __Level->GetWorld() : nullptr;
}

bool AActor::SetRootComponent(USceneComponent* _RootComponent)
{
	if (nullptr != _RootComponent && _RootComponent->GetOwner() != this)
		return false;

	__RootComponent = _RootComponent;
	return true;
}

FVector AActor::GetActorLocation() const
{
	return nullptr != __RootComponent
		? __RootComponent->GetWorldLocation()
		: FVector{};
}

bool AActor::SetActorLocation(const FVector& _Location)
{
	if (nullptr == __RootComponent)
		return false;

	__RootComponent->SetWorldLocation(_Location);
	return true;
}

void AActor::Tick(float _DeltaTime)
{
}

void AActor::PreInitializeComponents()
{
}

void AActor::PostInitializeComponents()
{
}

void AActor::BeginPlay()
{
}

void AActor::EndPlay()
{
}

void AActor::Destroy()
{
	if (UWorld* World = GetWorld())
		World->DestroyActor(this);
}

bool AActor::DestroyComponent(UActorComponent* _Component)
{
	if (nullptr == _Component)
		return false;

	if (_Component == __RootComponent)
		__RootComponent = nullptr;

	UWorld* World = GetWorld();

	if (World && World->IsTicking())
	{
		_Component->MarkPendingDestroy();
		World->QueueComponentDestroy(this, _Component);

		return true;
	}

	UnregisterComponentTickFunction(_Component);
	_Component->UnregisterComponent();
	RemoveComponent(_Component);

	return true;
}

void AActor::RegisterComponentTickFunction(UActorComponent* _Component)
{
	if (UWorld* World = GetWorld())
		World->RegisterComponentTickFunction(_Component);
}

void AActor::UnregisterComponentTickFunction(UActorComponent* _Component)
{
	if (UWorld* World = GetWorld())
		World->UnregisterComponentTickFunction(_Component);
}

void AActor::RemoveComponent(UActorComponent* _Component)
{
	auto Iter = std::remove_if
	(
		__Components.begin(),
		__Components.end(),
		[_Component](const std::unique_ptr<UActorComponent>& Component)
		{
			return Component.get() == _Component;
		}
	);

	__Components.erase(Iter, __Components.end());
}

void AActor::Destroyed()
{
}
