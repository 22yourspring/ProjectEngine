#pragma once

#include "PrimitiveSceneProxy.h"
#include "DynamicRHI.h"

class UStaticMesh;
class UMaterialInterface;
struct FMaterialRenderProxy;
struct FMaterialRenderData;

class FStaticMeshSceneProxy final : public FPrimitiveSceneProxy
{
public:
	explicit FStaticMeshSceneProxy(const UStaticMesh& _StaticMesh, UMaterialInterface* _Material = nullptr);
	virtual void Draw(FDynamicRHI& _DynamicRHI) const override;
    void SetTransform(const FTransform& _Transform) override { __Transform = _Transform; }

private:
	int32	__Width = 0;
	int32	__Height = 0;
	FColor	__Color = {};
    FTransform __Transform = FTransform::Identity;
    std::shared_ptr<FMaterialRenderProxy> __Material;
    mutable std::shared_ptr<const FMaterialRenderData> __Data;
    mutable FTextureRHIRef __Texture;
};
