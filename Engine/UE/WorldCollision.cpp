#include "pch.h"
#include "World.h"
#include "CollisionProfile.h"

bool UWorld::LineTraceMultiByChannel(std::vector<FHitResult>& _OutHits, const FVector& _Start, const FVector& _End, ECollisionChannel _TraceChannel, const FCollisionQueryParams& _Params, const FCollisionResponseParams& _ResponseParam) const
{
    _OutHits.clear();
    auto Params = _Params;
    const bool Result = __PhysicsScene->TraceMulti(_OutHits, _Start, _End, FQuat::Identity, nullptr, _TraceChannel, Params, _ResponseParam);
    return Result;
}

bool UWorld::LineTraceTestByChannel(const FVector& _Start, const FVector& _End, ECollisionChannel _TraceChannel, const FCollisionQueryParams& _Params, const FCollisionResponseParams& _ResponseParam) const
{
    std::vector<FHitResult> Hits;
    auto Params = _Params;
    Params.bIgnoreTouches = true;
    const bool Result = __PhysicsScene->TraceMulti(Hits, _Start, _End, FQuat::Identity, nullptr, _TraceChannel, Params, _ResponseParam);
    return Result;
}

bool UWorld::LineTraceSingleByObjectType(FHitResult& _OutHit, const FVector& _Start, const FVector& _End, const FCollisionObjectQueryParams& _ObjectQueryParams, const FCollisionQueryParams& _Params) const
{
    _OutHit = {}; _OutHit.TraceStart = _Start; _OutHit.TraceEnd = _End;
    std::vector<FHitResult> Hits;
    auto Params = _Params;
    Params.bIgnoreTouches = true;
    const bool Result = __PhysicsScene->TraceMulti(Hits, _Start, _End, FQuat::Identity, nullptr, ECC_WorldStatic, Params, FCollisionResponseParams::DefaultResponseParam, &_ObjectQueryParams);
    if (Result) _OutHit = Hits.front();
    return Result;
}

bool UWorld::LineTraceMultiByObjectType(std::vector<FHitResult>& _OutHits, const FVector& _Start, const FVector& _End, const FCollisionObjectQueryParams& _ObjectQueryParams, const FCollisionQueryParams& _Params) const
{
    _OutHits.clear();
    auto Params = _Params;
    const bool Result = __PhysicsScene->TraceMulti(_OutHits, _Start, _End, FQuat::Identity, nullptr, ECC_WorldStatic, Params, FCollisionResponseParams::DefaultResponseParam, &_ObjectQueryParams);
    return Result;
}

bool UWorld::LineTraceTestByObjectType(const FVector& _Start, const FVector& _End, const FCollisionObjectQueryParams& _ObjectQueryParams, const FCollisionQueryParams& _Params) const
{
    std::vector<FHitResult> Hits;
    auto Params = _Params;
    Params.bIgnoreTouches = true;
    const bool Result = __PhysicsScene->TraceMulti(Hits, _Start, _End, FQuat::Identity, nullptr, ECC_WorldStatic, Params, FCollisionResponseParams::DefaultResponseParam, &_ObjectQueryParams);
    return Result;
}

bool UWorld::LineTraceSingleByProfile(FHitResult& _OutHit, const FVector& _Start, const FVector& _End, FName _ProfileName, const FCollisionQueryParams& _Params) const
{
    _OutHit = {}; _OutHit.TraceStart = _Start; _OutHit.TraceEnd = _End;
    FCollisionResponseTemplate Profile;
    if (!UCollisionProfile::Get()->GetProfileTemplate(_ProfileName, Profile)) return false;
    std::vector<FHitResult> Hits;
    auto Params = _Params;
    Params.bIgnoreTouches = true;
    const bool Result = __PhysicsScene->TraceMulti(Hits, _Start, _End, FQuat::Identity, nullptr, Profile.ObjectType, Params, FCollisionResponseParams(Profile.ResponseToChannels));
    if (Result) _OutHit = Hits.front();
    return Result;
}

bool UWorld::LineTraceMultiByProfile(std::vector<FHitResult>& _OutHits, const FVector& _Start, const FVector& _End, FName _ProfileName, const FCollisionQueryParams& _Params) const
{
    _OutHits.clear();
    FCollisionResponseTemplate Profile;
    if (!UCollisionProfile::Get()->GetProfileTemplate(_ProfileName, Profile)) return false;
    auto Params = _Params;
    const bool Result = __PhysicsScene->TraceMulti(_OutHits, _Start, _End, FQuat::Identity, nullptr, Profile.ObjectType, Params, FCollisionResponseParams(Profile.ResponseToChannels));
    return Result;
}

bool UWorld::LineTraceTestByProfile(const FVector& _Start, const FVector& _End, FName _ProfileName, const FCollisionQueryParams& _Params) const
{
    FCollisionResponseTemplate Profile;
    if (!UCollisionProfile::Get()->GetProfileTemplate(_ProfileName, Profile)) return false;
    std::vector<FHitResult> Hits;
    auto Params = _Params;
    Params.bIgnoreTouches = true;
    const bool Result = __PhysicsScene->TraceMulti(Hits, _Start, _End, FQuat::Identity, nullptr, Profile.ObjectType, Params, FCollisionResponseParams(Profile.ResponseToChannels));
    return Result;
}

bool UWorld::SweepMultiByChannel(std::vector<FHitResult>& _OutHits, const FVector& _Start, const FVector& _End, const FQuat& _Rot, ECollisionChannel _TraceChannel, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params, const FCollisionResponseParams& _ResponseParam) const
{
    _OutHits.clear();
    auto Params = _Params;
    const bool Result = __PhysicsScene->TraceMulti(_OutHits, _Start, _End, _Rot, &_CollisionShape, _TraceChannel, Params, _ResponseParam);
    return Result;
}

bool UWorld::SweepTestByChannel(const FVector& _Start, const FVector& _End, const FQuat& _Rot, ECollisionChannel _TraceChannel, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params, const FCollisionResponseParams& _ResponseParam) const
{
    std::vector<FHitResult> Hits;
    auto Params = _Params;
    Params.bIgnoreTouches = true;
    const bool Result = __PhysicsScene->TraceMulti(Hits, _Start, _End, _Rot, &_CollisionShape, _TraceChannel, Params, _ResponseParam);
    return Result;
}

bool UWorld::SweepSingleByObjectType(FHitResult& _OutHit, const FVector& _Start, const FVector& _End, const FQuat& _Rot, const FCollisionObjectQueryParams& _ObjectQueryParams, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params) const
{
    _OutHit = {}; _OutHit.TraceStart = _Start; _OutHit.TraceEnd = _End;
    std::vector<FHitResult> Hits;
    auto Params = _Params;
    Params.bIgnoreTouches = true;
    const bool Result = __PhysicsScene->TraceMulti(Hits, _Start, _End, _Rot, &_CollisionShape, ECC_WorldStatic, Params, FCollisionResponseParams::DefaultResponseParam, &_ObjectQueryParams);
    if (Result) _OutHit = Hits.front();
    return Result;
}

bool UWorld::SweepMultiByObjectType(std::vector<FHitResult>& _OutHits, const FVector& _Start, const FVector& _End, const FQuat& _Rot, const FCollisionObjectQueryParams& _ObjectQueryParams, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params) const
{
    _OutHits.clear();
    auto Params = _Params;
    const bool Result = __PhysicsScene->TraceMulti(_OutHits, _Start, _End, _Rot, &_CollisionShape, ECC_WorldStatic, Params, FCollisionResponseParams::DefaultResponseParam, &_ObjectQueryParams);
    return Result;
}

bool UWorld::SweepTestByObjectType(const FVector& _Start, const FVector& _End, const FQuat& _Rot, const FCollisionObjectQueryParams& _ObjectQueryParams, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params) const
{
    std::vector<FHitResult> Hits;
    auto Params = _Params;
    Params.bIgnoreTouches = true;
    const bool Result = __PhysicsScene->TraceMulti(Hits, _Start, _End, _Rot, &_CollisionShape, ECC_WorldStatic, Params, FCollisionResponseParams::DefaultResponseParam, &_ObjectQueryParams);
    return Result;
}

bool UWorld::SweepSingleByProfile(FHitResult& _OutHit, const FVector& _Start, const FVector& _End, const FQuat& _Rot, FName _ProfileName, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params) const
{
    _OutHit = {}; _OutHit.TraceStart = _Start; _OutHit.TraceEnd = _End;
    FCollisionResponseTemplate Profile;
    if (!UCollisionProfile::Get()->GetProfileTemplate(_ProfileName, Profile)) return false;
    std::vector<FHitResult> Hits;
    auto Params = _Params;
    Params.bIgnoreTouches = true;
    const bool Result = __PhysicsScene->TraceMulti(Hits, _Start, _End, _Rot, &_CollisionShape, Profile.ObjectType, Params, FCollisionResponseParams(Profile.ResponseToChannels));
    if (Result) _OutHit = Hits.front();
    return Result;
}

bool UWorld::SweepMultiByProfile(std::vector<FHitResult>& _OutHits, const FVector& _Start, const FVector& _End, const FQuat& _Rot, FName _ProfileName, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params) const
{
    _OutHits.clear();
    FCollisionResponseTemplate Profile;
    if (!UCollisionProfile::Get()->GetProfileTemplate(_ProfileName, Profile)) return false;
    auto Params = _Params;
    const bool Result = __PhysicsScene->TraceMulti(_OutHits, _Start, _End, _Rot, &_CollisionShape, Profile.ObjectType, Params, FCollisionResponseParams(Profile.ResponseToChannels));
    return Result;
}

bool UWorld::SweepTestByProfile(const FVector& _Start, const FVector& _End, const FQuat& _Rot, FName _ProfileName, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params) const
{
    FCollisionResponseTemplate Profile;
    if (!UCollisionProfile::Get()->GetProfileTemplate(_ProfileName, Profile)) return false;
    std::vector<FHitResult> Hits;
    auto Params = _Params;
    Params.bIgnoreTouches = true;
    const bool Result = __PhysicsScene->TraceMulti(Hits, _Start, _End, _Rot, &_CollisionShape, Profile.ObjectType, Params, FCollisionResponseParams(Profile.ResponseToChannels));
    return Result;
}

bool UWorld::OverlapAnyTestByChannel(const FVector& _Pos, const FQuat& _Rot, ECollisionChannel _TraceChannel, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params, const FCollisionResponseParams& _ResponseParam) const
{
    std::vector<FOverlapResult> Overlaps;
    const bool Blocking = __PhysicsScene->OverlapMulti(Overlaps, _Pos, _Rot, _CollisionShape, _TraceChannel, _Params, _ResponseParam);
    return !Overlaps.empty();
}

bool UWorld::OverlapBlockingTestByChannel(const FVector& _Pos, const FQuat& _Rot, ECollisionChannel _TraceChannel, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params, const FCollisionResponseParams& _ResponseParam) const
{
    std::vector<FOverlapResult> Overlaps;
    const bool Blocking = __PhysicsScene->OverlapMulti(Overlaps, _Pos, _Rot, _CollisionShape, _TraceChannel, _Params, _ResponseParam);
    return Blocking;
}

bool UWorld::OverlapMultiByObjectType(std::vector<FOverlapResult>& _OutOverlaps, const FVector& _Pos, const FQuat& _Rot, const FCollisionObjectQueryParams& _ObjectQueryParams, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params) const
{
    _OutOverlaps.clear();
    const bool Blocking = __PhysicsScene->OverlapMulti(_OutOverlaps, _Pos, _Rot, _CollisionShape, ECC_WorldStatic, _Params, FCollisionResponseParams::DefaultResponseParam, &_ObjectQueryParams);
    return Blocking;
}

bool UWorld::OverlapAnyTestByObjectType(const FVector& _Pos, const FQuat& _Rot, const FCollisionObjectQueryParams& _ObjectQueryParams, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params) const
{
    std::vector<FOverlapResult> Overlaps;
    const bool Blocking = __PhysicsScene->OverlapMulti(Overlaps, _Pos, _Rot, _CollisionShape, ECC_WorldStatic, _Params, FCollisionResponseParams::DefaultResponseParam, &_ObjectQueryParams);
    return !Overlaps.empty();
}

bool UWorld::OverlapMultiByProfile(std::vector<FOverlapResult>& _OutOverlaps, const FVector& _Pos, const FQuat& _Rot, FName _ProfileName, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params) const
{
    _OutOverlaps.clear();
    FCollisionResponseTemplate Profile;
    if (!UCollisionProfile::Get()->GetProfileTemplate(_ProfileName, Profile)) return false;
    const bool Blocking = __PhysicsScene->OverlapMulti(_OutOverlaps, _Pos, _Rot, _CollisionShape, Profile.ObjectType, _Params, FCollisionResponseParams(Profile.ResponseToChannels));
    return Blocking;
}

bool UWorld::OverlapAnyTestByProfile(const FVector& _Pos, const FQuat& _Rot, FName _ProfileName, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params) const
{
    std::vector<FOverlapResult> Overlaps;
    FCollisionResponseTemplate Profile;
    if (!UCollisionProfile::Get()->GetProfileTemplate(_ProfileName, Profile)) return false;
    const bool Blocking = __PhysicsScene->OverlapMulti(Overlaps, _Pos, _Rot, _CollisionShape, Profile.ObjectType, _Params, FCollisionResponseParams(Profile.ResponseToChannels));
    return !Overlaps.empty();
}

bool UWorld::OverlapBlockingTestByProfile(const FVector& _Pos, const FQuat& _Rot, FName _ProfileName, const FCollisionShape& _CollisionShape, const FCollisionQueryParams& _Params) const
{
    std::vector<FOverlapResult> Overlaps;
    FCollisionResponseTemplate Profile;
    if (!UCollisionProfile::Get()->GetProfileTemplate(_ProfileName, Profile)) return false;
    const bool Blocking = __PhysicsScene->OverlapMulti(Overlaps, _Pos, _Rot, _CollisionShape, Profile.ObjectType, _Params, FCollisionResponseParams(Profile.ResponseToChannels));
    return Blocking;
}

