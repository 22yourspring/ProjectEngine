#include "pch.h"
#include "BoxComponent.h"
#include "SphereComponent.h"
#include "CapsuleComponent.h"
#include "World.h"
#include "PhysScene_Chaos.h"
#include "Scene.h"
#include <algorithm>
#include <cmath>

namespace
{
    void Changed(UPrimitiveComponent* _Component, bool _Overlaps)
    {
        _Component->BodyInstance.UpdateBodyScale(_Component->GetComponentScale(), true);
        if (_Component->GetWorld()) _Component->GetWorld()->GetScene()->AddPrimitive(_Component);
        if (_Overlaps && _Component->GetWorld()) _Component->GetWorld()->GetPhysicsScene()->UpdateOverlaps();
    }
}
void UBoxComponent::SetBoxExtent(FVector _InBoxExtent, bool _bUpdateOverlaps)
{
    if (_InBoxExtent.ContainsNaN()) return;
    __BoxExtent = _InBoxExtent.GetAbs().ComponentMax(FVector(0.01, 0.01, 0.01)); Changed(this, _bUpdateOverlaps);
}
FVector UBoxComponent::GetScaledBoxExtent() const { return __BoxExtent * GetComponentScale().GetAbs(); }
FCollisionShape UBoxComponent::GetCollisionShape(float _Inflation) const { return FCollisionShape::MakeBox((GetScaledBoxExtent() + FVector(_Inflation)).ComponentMax(FVector(0.01))); }
void USphereComponent::SetSphereRadius(float _InSphereRadius, bool _bUpdateOverlaps)
{
    if (!std::isfinite(_InSphereRadius)) return;
    __SphereRadius = (std::max)(0.01f, _InSphereRadius); Changed(this, _bUpdateOverlaps);
}
float USphereComponent::GetScaledSphereRadius() const { return __SphereRadius * static_cast<float>(GetComponentScale().GetAbs().GetMin()); }
FCollisionShape USphereComponent::GetCollisionShape(float _Inflation) const { return FCollisionShape::MakeSphere((std::max)(0.01f, GetScaledSphereRadius() + _Inflation)); }
void UCapsuleComponent::SetCapsuleSize(float _InRadius, float _InHalfHeight, bool _bUpdateOverlaps)
{
    if (!std::isfinite(_InRadius) || !std::isfinite(_InHalfHeight)) return;
    __CapsuleRadius = (std::max)(0.01f, _InRadius); __CapsuleHalfHeight = (std::max)(__CapsuleRadius, _InHalfHeight); Changed(this, _bUpdateOverlaps);
}
float UCapsuleComponent::GetScaledCapsuleRadius() const { const auto Scale = GetComponentScale().GetAbs(); return __CapsuleRadius * static_cast<float>((std::max)(Scale.X, Scale.Y)); }
float UCapsuleComponent::GetScaledCapsuleHalfHeight() const { return (std::max)(GetScaledCapsuleRadius(), __CapsuleHalfHeight * static_cast<float>(std::abs(GetComponentScale().Z))); }
FCollisionShape UCapsuleComponent::GetCollisionShape(float _Inflation) const { return FCollisionShape::MakeCapsule((std::max)(0.01f, GetScaledCapsuleRadius() + _Inflation), (std::max)(0.01f, GetScaledCapsuleHalfHeight() + _Inflation)); }
