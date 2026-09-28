#pragma once
#include "MaterialInstance.h"

UCLASS(MinimalAPI)
class ENGINE_API UMaterialInstanceConstant : public UMaterialInstance
{
    GENERATED_BODY()
public:
    bool SetParentEditorOnly(UMaterialInterface* _Parent) { return SetParentInternal(_Parent); }
    void SetTextureParameterValueEditorOnly(FName _Name, UTexture* _Value) { SetTextureParameterValueInternal(_Name, _Value); }
    void SetVectorParameterValueEditorOnly(FName _Name, FLinearColor _Value) { SetVectorParameterValueInternal(_Name, _Value); }
    void SetScalarParameterValueEditorOnly(FName _Name, float _Value) { SetScalarParameterValueInternal(_Name, _Value); }
};
