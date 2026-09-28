#pragma once
#include "StreamableRenderAsset.h"

UCLASS(Abstract, MinimalAPI)
class ENGINE_API UTexture : public UStreamableRenderAsset
{
    GENERATED_BODY()

public:
    virtual uint32 GetSizeX() const = 0;
    virtual uint32 GetSizeY() const = 0;
};
