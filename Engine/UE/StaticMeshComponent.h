#pragma once

#include "MeshComponent.h"

class UStaticMesh;
struct FColor;

UCLASS(MinimalAPI)
class ENGINE_API UStaticMeshComponent : public UMeshComponent
{
	GENERATED_BODY()

public:
	bool SetStaticMesh(UStaticMesh* _StaticMesh);
	UStaticMesh* GetStaticMesh() const { return __StaticMesh; }
    void RestoreMesh(int32 _Width, int32 _Height, const FColor& _Color);
	virtual FPrimitiveSceneProxy* CreateSceneProxy() const override;

private:
	UStaticMesh* __StaticMesh = nullptr;
    std::shared_ptr<UStaticMesh> __RestoredMesh;
};
