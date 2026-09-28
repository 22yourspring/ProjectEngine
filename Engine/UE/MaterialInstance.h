#pragma once
#include "MaterialInterface.h"

UCLASS(Abstract, MinimalAPI)
class ENGINE_API UMaterialInstance : public UMaterialInterface
{
    GENERATED_BODY()
public:
    ~UMaterialInstance() override;
    UMaterial* GetMaterial() override;
    UMaterialInterface* GetParent() const { return __Parent; }
    bool GetTextureParameterValue(FName _Name, UTexture*& _Value) const override;
    bool GetVectorParameterValue(FName _Name, FLinearColor& _Value) const override;
    bool GetScalarParameterValue(FName _Name, float& _Value) const override;
    void ClearParameterValues();
    bool HasTextureOverride() const { return __TextureOverride; }
    bool HasVectorOverride() const { return __ColorOverride; }
    bool HasScalarOverride() const { return __OpacityOverride; }
    void Serialize(FArchive& _Archive) override;

protected:
    bool SetParentInternal(UMaterialInterface* _Parent);
    void SetTextureParameterValueInternal(FName _Name, UTexture* _Value);
    void SetVectorParameterValueInternal(FName _Name, FLinearColor _Value);
    void SetScalarParameterValueInternal(FName _Name, float _Value);

private:
    UMaterialInterface* __Parent = nullptr;
    UTexture* __Texture = nullptr;
    FLinearColor __BaseColor = FLinearColor(1, 1, 1, 1);
    float __Opacity = 1;
    bool __TextureOverride = false;
    bool __ColorOverride = false;
    bool __OpacityOverride = false;
};
