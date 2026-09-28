#pragma once
#include "ShapeComponent.h"

UCLASS(MinimalAPI)
class ENGINE_API UBoxComponent : public UShapeComponent
{
public:
    static UClass* StaticClass() { return UClass::For<UBoxComponent>(); }
    void SetBoxExtent(FVector _InBoxExtent, bool _bUpdateOverlaps = true);
    FVector GetUnscaledBoxExtent() const { return __BoxExtent; }
    FVector GetScaledBoxExtent() const;
    FCollisionShape GetCollisionShape(float _Inflation = 0.0f) const override;
private:
    FVector __BoxExtent = FVector(32, 32, 32);
};
