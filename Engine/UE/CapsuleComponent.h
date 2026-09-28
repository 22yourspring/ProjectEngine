#pragma once
#include "ShapeComponent.h"

UCLASS(MinimalAPI)
class ENGINE_API UCapsuleComponent : public UShapeComponent
{
public:
    static UClass* StaticClass() { return UClass::For<UCapsuleComponent>(); }
    void SetCapsuleSize(float _InRadius, float _InHalfHeight, bool _bUpdateOverlaps = true);
    void SetCapsuleRadius(float _Radius, bool _bUpdateOverlaps = true) { SetCapsuleSize(_Radius, __CapsuleHalfHeight, _bUpdateOverlaps); }
    void SetCapsuleHalfHeight(float _HalfHeight, bool _bUpdateOverlaps = true) { SetCapsuleSize(__CapsuleRadius, _HalfHeight, _bUpdateOverlaps); }
    float GetUnscaledCapsuleRadius() const { return __CapsuleRadius; }
    float GetUnscaledCapsuleHalfHeight() const { return __CapsuleHalfHeight; }
    float GetScaledCapsuleRadius() const;
    float GetScaledCapsuleHalfHeight() const;
    FCollisionShape GetCollisionShape(float _Inflation = 0.0f) const override;
private:
    float __CapsuleRadius = 22;
    float __CapsuleHalfHeight = 44;
};
