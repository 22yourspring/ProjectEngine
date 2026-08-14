#include "pch.h"
#include "ActorComponent.h"
#include "Actor.h"

void FComponentTickFunction::ExecuteTick(UActorComponent* _Target, float _DeltaTime)
{
	if (nullptr == _Target || false == bCanEverTick || _Target->IsPendingDestroy())
		return;

	if (AActor* Owner = dynamic_cast<AActor*>(_Target->GetOwner()))
	{
		if (Owner->IsPendingDestroy())
			return;
	}

	_Target->TickComponent(_DeltaTime);
}

void UActorComponent::TickComponent(float _DeltaTime)
{
}

void UActorComponent::BeginPlay()
{
}

void UActorComponent::EndPlay()
{
}

void UActorComponent::InitializeComponent()
{
}

void UActorComponent::DestroyComponent()
{
	MarkPendingDestroy();

	if (AActor* Owner = dynamic_cast<AActor*>(__Owner))
		Owner->DestroyComponent(this);
}

void UActorComponent::OnRegister()
{
}

void UActorComponent::OnUnregister()
{
}

void UActorComponent::RegisterComponentWithWorld(UWorld* _World)
{
	if (nullptr == _World || nullptr != __World)
		return;
	__World = _World;
	OnRegister();
}

void UActorComponent::UnregisterComponent()
{
	if (nullptr == __World)
		return;
	OnUnregister();
	__World = nullptr;
}
