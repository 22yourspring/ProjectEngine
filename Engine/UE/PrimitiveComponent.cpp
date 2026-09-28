#include "pch.h"
#include "PrimitiveComponent.h"
#include "World.h"
#include "Scene.h"
#include "MaterialInstanceDynamic.h"
#include "PhysScene_Chaos.h"

UMaterialInstanceDynamic* UPrimitiveComponent::CreateDynamicMaterialInstance(int32 _ElementIndex, UMaterialInterface* _SourceMaterial, FName _OptionalName)
{
    if (_ElementIndex < 0 || _ElementIndex >= GetNumMaterials()) return nullptr;
    auto* Source = _SourceMaterial ? _SourceMaterial : GetMaterial(_ElementIndex);
    if (!Source) return nullptr;
    auto* Instance = dynamic_cast<UMaterialInstanceDynamic*>(Source);
    if (!Instance) Instance = UMaterialInstanceDynamic::Create(Source, this, _OptionalName);
    SetMaterial(_ElementIndex, Instance);
    return Instance;
}

UPrimitiveComponent::~UPrimitiveComponent() { UnregisterComponent(); }

void UPrimitiveComponent::SetCollisionProfileName(FName _InCollisionProfileName, bool _bUpdateOverlaps)
{
    BodyInstance.SetCollisionProfileName(_InCollisionProfileName);
    if (_bUpdateOverlaps && GetWorld()) GetWorld()->GetPhysicsScene()->UpdateOverlaps();
}

void UPrimitiveComponent::OnRegister()
{
	UActorComponent::OnRegister();
	BodyInstance.InitBody(nullptr, GetComponentTransform(), this, GetWorld() ? GetWorld()->GetPhysicsScene() : nullptr);
	if (UWorld* World = GetWorld())
		World->GetScene()->AddPrimitive(this);
}

void UPrimitiveComponent::OnUnregister()
{
	BodyInstance.TermBody();
	if (UWorld* World = GetWorld())
		World->GetScene()->RemovePrimitive(this);
	UActorComponent::OnUnregister();
}

void UPrimitiveComponent::OnUpdateTransform()
{
	if (UWorld* World = GetWorld()) World->GetPhysicsScene()->UpdateTransform(&BodyInstance);
	if (UWorld* World = GetWorld())
		World->GetScene()->UpdatePrimitiveTransform(this, GetWorldLocation());
}
