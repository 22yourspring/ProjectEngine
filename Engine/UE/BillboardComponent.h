#pragma once
#include "PrimitiveComponent.h"

class UTexture2D;

UCLASS(MinimalAPI)
class ENGINE_API UBillboardComponent final : public UPrimitiveComponent
{
    GENERATED_BODY()

public:
    void SetSprite(UTexture2D* _Sprite);
    void SetSize(int32 _Width, int32 _Height);
    UTexture2D* GetSprite() const { return __Sprite; }
    int32 GetSpriteWidth() const { return __Width; }
    int32 GetSpriteHeight() const { return __Height; }
    FPrimitiveSceneProxy* CreateSceneProxy() const override;

private:
    void RefreshSprite();
    UTexture2D* __Sprite = nullptr;
    int32 __Width = 0;
    int32 __Height = 0;
};
