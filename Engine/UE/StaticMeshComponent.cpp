#include "pch.h"
#include "StaticMesh.h"
#include "StaticMeshComponent.h"
#include "StaticMeshSceneProxy.h"
#include "World.h"
#include "Scene.h"

void UStaticMeshComponent::RestoreMesh(int32 _Width, int32 _Height, const FColor& _Color)
{
    if (!__StaticMesh) { __RestoredMesh = std::make_shared<UStaticMesh>(); __StaticMesh = __RestoredMesh.get(); }
    __StaticMesh->SetSize(_Width, _Height);
    __StaticMesh->SetColor(_Color);
    SetStaticMesh(__StaticMesh);
    if (auto* World = GetWorld()) { UnregisterComponent(); RegisterComponentWithWorld(World); }
}

bool UStaticMeshComponent::SetStaticMesh(UStaticMesh* _StaticMesh)
{
	__StaticMesh = _StaticMesh;
    if (auto* World = GetWorld())
    {
        World->GetScene()->RemovePrimitive(this);
        if (__StaticMesh) World->GetScene()->AddPrimitive(this);
    }
	return nullptr != __StaticMesh;
}

FPrimitiveSceneProxy* UStaticMeshComponent::CreateSceneProxy() const
{
	if (nullptr == __StaticMesh)
		return nullptr;
	return new FStaticMeshSceneProxy(*__StaticMesh, GetMaterial(0));
}
