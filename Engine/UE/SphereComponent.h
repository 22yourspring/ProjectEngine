#pragma once
#include "ShapeComponent.h"

UCLASS(MinimalAPI)
class ENGINE_API USphereComponent : public UShapeComponent
{
public:
    static UClass* StaticClass() { return UClass::For<USphereComponent>(); }
    void SetSphereRadius(float _InSphereRadius, bool _bUpdateOverlaps = true);
    float GetUnscaledSphereRadius() const { return __SphereRadius; }
    float GetScaledSphereRadius() const;
    FCollisionShape GetCollisionShape(float _Inflation = 0.0f) const override;
private:
    float __SphereRadius = 32;
};
