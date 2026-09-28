#include "pch.h"
#include "BillboardComponent.h"
#include "Texture2D.h"
#include "PrimitiveSceneProxy.h"
#include "DynamicRHI.h"
#include "World.h"
#include "Scene.h"

namespace
{
    class FBillboardSceneProxy final : public FPrimitiveSceneProxy
    {
    public:
        FBillboardSceneProxy(const UTexture2D& _Texture, int32 _Width, int32 _Height)
            : __Pixels(_Texture.GetPixels()), __SizeX(_Texture.GetSizeX()), __SizeY(_Texture.GetSizeY()),
              __Width(_Width > 0 ? _Width : static_cast<int32>(__SizeX)),
              __Height(_Height > 0 ? _Height : static_cast<int32>(__SizeY)) {}

        void Draw(FDynamicRHI& _DynamicRHI) const override
        {
            if (!__Texture) __Texture = _DynamicRHI.RHICreateTexture2D(__SizeX, __SizeY, __Pixels);
            if (__Texture) _DynamicRHI.RHIDrawTexture(__Texture.get(), static_cast<int32>(__WorldLocation.X),
                static_cast<int32>(__WorldLocation.Y), __Width, __Height);
        }

    private:
        std::vector<uint8> __Pixels;
        uint32 __SizeX, __SizeY;
        int32 __Width, __Height;
        mutable FTextureRHIRef __Texture;
    };
}

void UBillboardComponent::SetSprite(UTexture2D* _Sprite)
{
    __Sprite = _Sprite;
    RefreshSprite();
}

void UBillboardComponent::SetSize(int32 _Width, int32 _Height)
{
    __Width = _Width > 0 ? _Width : 0;
    __Height = _Height > 0 ? _Height : 0;
    RefreshSprite();
}

void UBillboardComponent::RefreshSprite()
{
    if (auto* World = GetWorld())
    {
        World->GetScene()->RemovePrimitive(this);
        if (__Sprite) World->GetScene()->AddPrimitive(this);
    }
}

FPrimitiveSceneProxy* UBillboardComponent::CreateSceneProxy() const
{
    return __Sprite ? new FBillboardSceneProxy(*__Sprite, __Width, __Height) : nullptr;
}
