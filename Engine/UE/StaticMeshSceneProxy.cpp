#include "pch.h"
#include "StaticMeshSceneProxy.h"
#include "StaticMesh.h"
#include "MaterialInterface.h"

FStaticMeshSceneProxy::FStaticMeshSceneProxy(const UStaticMesh& _StaticMesh, UMaterialInterface* _Material)
	: __Width(_StaticMesh.GetWidth()),
	  __Height(_StaticMesh.GetHeight()),
	  __Color(_StaticMesh.GetColor()), __Material(_Material ? _Material->GetRenderProxy() : nullptr)
{
}

void FStaticMeshSceneProxy::Draw(FDynamicRHI& _DynamicRHI) const
{
    FVector Corners[] = {FVector(0, 0, 0), FVector(__Width, 0, 0), FVector(__Width, __Height, 0), FVector(0, __Height, 0)};
    for (auto& Corner : Corners) Corner = __Transform.TransformPosition(Corner);
    if (__Material)
    {
        std::shared_ptr<const FMaterialRenderData> Data;
        { std::lock_guard<std::mutex> Lock(__Material->__Mutex); Data = __Material->__Data; }
        if (Data != __Data || !__Texture)
        {
            __Texture = _DynamicRHI.RHICreateTexture2D(Data->__Width, Data->__Height, Data->__Pixels);
            __Data = std::move(Data);
        }
        if (__Texture) _DynamicRHI.RHIDrawQuad(Corners, FColor(255, 255, 255), __Texture.get());
        return;
    }
    _DynamicRHI.RHIDrawQuad(Corners, __Color);
}
