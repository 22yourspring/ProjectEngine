#include "pch.h"
#include "MeshComponent.h"
#include "World.h"
#include "Scene.h"

UMaterialInterface* UMeshComponent::GetMaterial(int32 _ElementIndex) const
{
    return _ElementIndex == 0 ? __Material : nullptr;
}

void UMeshComponent::SetMaterial(int32 _ElementIndex, UMaterialInterface* _Material)
{
    if (_ElementIndex != 0) return;
    __Material = _Material;
    if (auto* World = GetWorld())
    {
        World->GetScene()->RemovePrimitive(this);
        World->GetScene()->AddPrimitive(this);
    }
}
