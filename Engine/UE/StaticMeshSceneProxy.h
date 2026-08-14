#pragma once

#include "PrimitiveSceneProxy.h"
#include "DynamicRHI.h"

class UStaticMesh;

class FStaticMeshSceneProxy final : public FPrimitiveSceneProxy
{
public:
	explicit FStaticMeshSceneProxy(const UStaticMesh& _StaticMesh);
	virtual void Draw(FDynamicRHI& _DynamicRHI) const override;

private:
	int32	__Width = 0;
	int32	__Height = 0;
	FColor	__Color = {};
};
