#pragma once
#include "MaterialInstance.h"

UCLASS(MinimalAPI)
class ENGINE_API UMaterialInstanceDynamic : public UMaterialInstance
{
    GENERATED_BODY()
public:
    static UMaterialInstanceDynamic* Create(UMaterialInterface* _ParentMaterial, UObject* _Outer, FName _Name = FName());
    void SetTextureParameterValue(FName _Name, UTexture* _Value) { SetTextureParameterValueInternal(_Name, _Value); }
    void SetVectorParameterValue(FName _Name, FLinearColor _Value) { SetVectorParameterValueInternal(_Name, _Value); }
    void SetScalarParameterValue(FName _Name, float _Value) { SetScalarParameterValueInternal(_Name, _Value); }
};
