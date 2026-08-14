#include "pch.h"
#include "StaticMeshComponent.h"
#include "StaticMesh.h"
#include "StaticMeshSceneProxy.h"

bool UStaticMeshComponent::SetStaticMesh(UStaticMesh* _StaticMesh)
{
	__StaticMesh = _StaticMesh;
	return nullptr != __StaticMesh;
}

FPrimitiveSceneProxy* UStaticMeshComponent::CreateSceneProxy() const
{
	if (nullptr == __StaticMesh)
		return nullptr;
	return new FStaticMeshSceneProxy(*__StaticMesh);
}
