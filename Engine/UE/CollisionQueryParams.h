#pragma once
#include "EngineTypes.h"
#include "NameTypes.h"
#include "Math/Vector.h"
#include <vector>
#include <algorithm>

class AActor;
class UPrimitiveComponent;
struct FCollisionObjectQueryParams
{
    FCollisionObjectQueryParams() = default;
    explicit FCollisionObjectQueryParams(ECollisionChannel _Channel) { AddObjectTypesToQuery(_Channel); }
    void AddObjectTypesToQuery(ECollisionChannel _Channel) { if (_Channel >= 0 && _Channel < 32) __ObjectTypes |= uint32(1) << _Channel; }
    void RemoveObjectTypesToQuery(ECollisionChannel _Channel) { if (_Channel >= 0 && _Channel < 32) __ObjectTypes &= ~(uint32(1) << _Channel); }
    bool IsValid() const { return __ObjectTypes != 0; }
    bool Contains(ECollisionChannel _Channel) const { return _Channel >= 0 && _Channel < 32 && (__ObjectTypes & (uint32(1) << _Channel)) != 0; }
    uint32 GetQueryBitfield() const { return __ObjectTypes; }
private:
    uint32 __ObjectTypes = 0;
};
struct FCollisionQueryParams
{
    FCollisionQueryParams() = default;
    FCollisionQueryParams(FName _TraceTag, bool _bTraceComplex = false, const AActor* _InIgnoreActor = nullptr)
        : TraceTag(_TraceTag), bTraceComplex(_bTraceComplex) { AddIgnoredActor(_InIgnoreActor); }
    void AddIgnoredActor(const AActor* _Actor) { if (_Actor) __IgnoredActors.push_back(_Actor); }
    void AddIgnoredComponent(const UPrimitiveComponent* _Component) { if (_Component) __IgnoredComponents.push_back(_Component); }
    bool IsIgnored(const AActor* _Actor, const UPrimitiveComponent* _Component) const
    {
        return std::find(__IgnoredActors.begin(), __IgnoredActors.end(), _Actor) != __IgnoredActors.end() ||
            std::find(__IgnoredComponents.begin(), __IgnoredComponents.end(), _Component) != __IgnoredComponents.end();
    }
    FName TraceTag;
    bool bTraceComplex = false;
    bool bFindInitialOverlaps = true;
    bool bIgnoreBlocks = false;
    bool bIgnoreTouches = false;
    static ENGINE_API const FCollisionQueryParams DefaultQueryParam;
private:
    std::vector<const AActor*> __IgnoredActors;
    std::vector<const UPrimitiveComponent*> __IgnoredComponents;
};
struct FCollisionResponseParams
{
    explicit FCollisionResponseParams(ECollisionResponse _Response = ECR_Block) : CollisionResponse(_Response) {}
    explicit FCollisionResponseParams(const FCollisionResponseContainer& _Response) : CollisionResponse(_Response) {}
    FCollisionResponseContainer CollisionResponse;
    static ENGINE_API const FCollisionResponseParams DefaultResponseParam;
};
struct FHitResult
{
    bool bBlockingHit = false;
    bool bStartPenetrating = false;
    float Time = 1.0f;
    float Distance = 0.0f;
    float PenetrationDepth = 0.0f;
    FVector Location = FVector::ZeroVector;
    FVector ImpactPoint = FVector::ZeroVector;
    FVector Normal = FVector::ZeroVector;
    FVector ImpactNormal = FVector::ZeroVector;
    FVector TraceStart = FVector::ZeroVector;
    FVector TraceEnd = FVector::ZeroVector;
    UPrimitiveComponent* Component = nullptr;
    ENGINE_API AActor* GetActor() const;
    UPrimitiveComponent* GetComponent() const { return Component; }
};
struct FOverlapResult
{
    bool bBlockingHit = false;
    UPrimitiveComponent* Component = nullptr;
    ENGINE_API AActor* GetActor() const;
    UPrimitiveComponent* GetComponent() const { return Component; }
};
