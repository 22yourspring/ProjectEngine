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
	if (nullptr != _RootComponent && !OwnsComponent(_RootComponent))
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
	if (!OwnsComponent(_Component))
		return false;

	UWorld* World = GetWorld();

	if (World && World->IsTicking())
	{
		if (_Component->IsPendingDestroy())
			return false;

		_Component->MarkPendingDestroy();
		World->QueueComponentDestroy(this, _Component);

		return true;
	}

	_Component->MarkPendingDestroy();

	if (_Component == __RootComponent)
		__RootComponent = nullptr;

	UnregisterComponentTickFunction(_Component);
	_Component->UnregisterComponent();
	RemoveComponent(_Component);

	return true;
}

bool AActor::OwnsComponent(UActorComponent* _Component) const
{
	return nullptr != _Component &&
		__OwnedComponents.contains(_Component);
}

bool AActor::AddOwnedComponent(UActorComponent* _Component)
{
	if (nullptr == _Component || _Component->GetOwner() != this)
		return false;

	return __OwnedComponents.insert(_Component).second;
}

bool AActor::AddInstanceComponent(UActorComponent* _Component)
{
	if (!OwnsComponent(_Component))
		return false;

	if (std::find(
		__InstanceComponents.begin(),
		__InstanceComponents.end(),
		_Component) != __InstanceComponents.end())
	{
		return false;
	}

	__InstanceComponents.push_back(_Component);
	return true;
}

void AActor::RemoveOwnedComponent(UActorComponent* _Component)
{
	__OwnedComponents.erase(_Component);
}

void AActor::RemoveInstanceComponent(UActorComponent* _Component)
{
	__InstanceComponents.erase(
		std::remove(
			__InstanceComponents.begin(),
			__InstanceComponents.end(),
			_Component),
		__InstanceComponents.end());
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
	RemoveInstanceComponent(_Component);
	RemoveOwnedComponent(_Component);
	_Component->SetOwner(nullptr);

	auto Iter = std::remove_if
	(
		__ComponentStorage.begin(),
		__ComponentStorage.end(),
		[_Component](const std::unique_ptr<UActorComponent>& Component)
		{
			return Component.get() == _Component;
		}
	);

	__ComponentStorage.erase(Iter, __ComponentStorage.end());
}

void AActor::Destroyed()
{
}
