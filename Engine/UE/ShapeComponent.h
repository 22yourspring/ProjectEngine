#pragma once
#include "PrimitiveComponent.h"

UCLASS(Abstract, MinimalAPI)
class ENGINE_API UShapeComponent : public UPrimitiveComponent
{
public:
    UShapeComponent() { SetCollisionProfileName(FName(TEXT("OverlapAllDynamic"))); }
};
