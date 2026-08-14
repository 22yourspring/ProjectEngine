#pragma once

#include "PrimitiveComponent.h"

class UStaticMesh;

UCLASS(MinimalAPI)
class UStaticMeshComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	bool SetStaticMesh(UStaticMesh* _StaticMesh);
	UStaticMesh* GetStaticMesh() const { return __StaticMesh; }
	virtual FPrimitiveSceneProxy* CreateSceneProxy() const override;

private:
	UStaticMesh* __StaticMesh = nullptr;
};
