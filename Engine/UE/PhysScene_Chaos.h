#pragma once
#include "PhysicsInterface.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include <memory>

struct FBodyInstance;
class ENGINE_API FPhysScene_Chaos
{
public:
    explicit FPhysScene_Chaos(AActor* _InSolverActor = nullptr);
    ~FPhysScene_Chaos();
    FPhysScene_Chaos(const FPhysScene_Chaos&) = delete;
    FPhysScene_Chaos& operator=(const FPhysScene_Chaos&) = delete;
    AActor* GetSolverActor() const;
    void SetUpForFrame(const FVector* _NewGrav, float _InDeltaSeconds, float _InMinPhysicsDeltaTime, float _InMaxPhysicsDeltaTime, float _InMaxSubstepDeltaTime, int32 _InMaxSubsteps, bool _bSubstepping);
    void StartFrame();
    void EndFrame();
    void SetGravity(const FVector& _Gravity);
    void AddBody(FBodyInstance* _Body);
    void RemoveBody(FBodyInstance* _Body, bool _bUnregister = false);
    void UpdateTransform(FBodyInstance* _Body);
    bool LineTraceSingleByChannel(FHitResult& _OutHit, const FVector& _Start, const FVector& _End, ECollisionChannel _TraceChannel, const FCollisionQueryParams& _Params, const FCollisionResponseParams& _ResponseParam) const;
    bool SweepSingleByChannel(FHitResult& _OutHit, const FVector& _Start, const FVector& _End, const FQuat& _Rot, ECollisionChannel _TraceChannel, const FCollisionShape& _Shape, const FCollisionQueryParams& _Params, const FCollisionResponseParams& _ResponseParam) const;
    bool OverlapMultiByChannel(std::vector<FOverlapResult>& _OutOverlaps, const FVector& _Pos, const FQuat& _Rot, ECollisionChannel _TraceChannel, const FCollisionShape& _Shape, const FCollisionQueryParams& _Params, const FCollisionResponseParams& _ResponseParam) const;
    void UpdateOverlaps();
    bool TraceMulti(std::vector<FHitResult>& _OutHits, const FVector& _Start, const FVector& _End, const FQuat& _Rot, const FCollisionShape* _Shape, ECollisionChannel _Channel, const FCollisionQueryParams& _Params, const FCollisionResponseParams& _Response, const FCollisionObjectQueryParams* _Objects = nullptr) const;
    bool OverlapMulti(std::vector<FOverlapResult>& _OutOverlaps, const FVector& _Pos, const FQuat& _Rot, const FCollisionShape& _Shape, ECollisionChannel _Channel, const FCollisionQueryParams& _Params, const FCollisionResponseParams& _Response, const FCollisionObjectQueryParams* _Objects = nullptr) const;
private:
    void Tick(float _DeltaTime);
    void DispatchEvents();
    struct FImpl;
    std::unique_ptr<FImpl> __Impl;
};
