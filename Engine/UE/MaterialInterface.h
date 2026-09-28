#pragma once
#include "Object.h"
#include "Math/Color.h"
#include <mutex>

class UTexture;
class UMaterial;

struct FMaterialRenderData
{
    uint32 __Width = 1;
    uint32 __Height = 1;
    std::vector<uint8> __Pixels = {255, 255, 255, 255};
};

struct FMaterialRenderProxy
{
    std::mutex __Mutex;
    std::shared_ptr<const FMaterialRenderData> __Data = std::make_shared<FMaterialRenderData>();
};

UCLASS(Abstract, MinimalAPI)
class ENGINE_API UMaterialInterface : public UObject
{
    GENERATED_BODY()
public:
    UMaterialInterface();
    ~UMaterialInterface() override;
    virtual UMaterial* GetMaterial() = 0;
    virtual bool GetTextureParameterValue(FName _Name, UTexture*& _Value) const = 0;
    virtual bool GetVectorParameterValue(FName _Name, FLinearColor& _Value) const = 0;
    virtual bool GetScalarParameterValue(FName _Name, float& _Value) const = 0;
    std::shared_ptr<FMaterialRenderProxy> GetRenderProxy() const { return __RenderProxy; }
    static void RefreshAllMaterials();
    void PostLoad() override;

protected:
    void MaterialChanged();
    void UnregisterMaterial();

private:
    std::shared_ptr<FMaterialRenderProxy> __RenderProxy;
};
