#pragma once
#include "PrimitiveComponent.h"

UCLASS(Abstract, MinimalAPI)
class ENGINE_API UMeshComponent : public UPrimitiveComponent
{
    GENERATED_BODY()
public:
    UMaterialInterface* GetMaterial(int32 _ElementIndex) const override;
    void SetMaterial(int32 _ElementIndex, UMaterialInterface* _Material) override;
    int32 GetNumMaterials() const override { return 1; }

private:
    UMaterialInterface* __Material = nullptr;
};
