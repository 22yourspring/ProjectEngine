#pragma once
#include "MaterialInterface.h"

UCLASS(MinimalAPI)
class ENGINE_API UMaterial : public UMaterialInterface
{
    GENERATED_BODY()
public:
    ~UMaterial() override;
    UMaterial* GetMaterial() override { return this; }
    bool GetTextureParameterValue(FName _Name, UTexture*& _Value) const override;
    bool GetVectorParameterValue(FName _Name, FLinearColor& _Value) const override;
    bool GetScalarParameterValue(FName _Name, float& _Value) const override;
    bool SetTextureParameterValueEditorOnly(FName _Name, UTexture* _Value);
    bool SetVectorParameterValueEditorOnly(FName _Name, FLinearColor _Value);
    bool SetScalarParameterValueEditorOnly(FName _Name, float _Value);
    void Serialize(FArchive& _Archive) override;

private:
    UTexture* __Texture = nullptr;
    FLinearColor __BaseColor = FLinearColor(1, 1, 1, 1);
    float __Opacity = 1;
};
