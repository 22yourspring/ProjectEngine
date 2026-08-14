#include "pch.h"
#include "PrimitiveComponent.h"
#include "World.h"
#include "Scene.h"

UPrimitiveComponent::~UPrimitiveComponent() = default;

void UPrimitiveComponent::OnRegister()
{
	UActorComponent::OnRegister();
	if (UWorld* World = GetWorld())
		World->GetScene()->AddPrimitive(this);
}

void UPrimitiveComponent::OnUnregister()
{
	if (UWorld* World = GetWorld())
		World->GetScene()->RemovePrimitive(this);
	UActorComponent::OnUnregister();
}

void UPrimitiveComponent::OnUpdateTransform()
{
	if (UWorld* World = GetWorld())
		World->GetScene()->UpdatePrimitiveTransform(this, GetWorldLocation());
}
