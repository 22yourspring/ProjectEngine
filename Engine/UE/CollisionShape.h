#pragma once
#include "Math/Vector.h"

namespace ECollisionShape { enum Type { Line, Box, Sphere, Capsule }; }
struct FCollisionShape
{
    static FCollisionShape MakeBox(const FVector& _HalfExtent) { return {ECollisionShape::Box, _HalfExtent}; }
    static FCollisionShape MakeSphere(float _Radius) { return {ECollisionShape::Sphere, FVector(_Radius, 0, 0)}; }
    static FCollisionShape MakeCapsule(float _Radius, float _HalfHeight) { return {ECollisionShape::Capsule, FVector(_Radius, _HalfHeight, 0)}; }
    FVector GetBox() const { return __Dimensions; }
    float GetSphereRadius() const { return static_cast<float>(__Dimensions.X); }
    float GetCapsuleRadius() const { return static_cast<float>(__Dimensions.X); }
    float GetCapsuleHalfHeight() const { return static_cast<float>(__Dimensions.Y); }
    bool IsBox() const { return __Type == ECollisionShape::Box; }
    bool IsSphere() const { return __Type == ECollisionShape::Sphere; }
    bool IsCapsule() const { return __Type == ECollisionShape::Capsule; }
    bool IsNearlyZero() const { return __Type == ECollisionShape::Line; }
private:
    FCollisionShape(ECollisionShape::Type _Type, FVector _Dimensions) : __Type(_Type), __Dimensions(_Dimensions) {}
    ECollisionShape::Type __Type;
    FVector __Dimensions;
public:
    FCollisionShape() : __Type(ECollisionShape::Line), __Dimensions(FVector::ZeroVector) {}
};
