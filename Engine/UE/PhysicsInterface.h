#pragma once
#include "Math/Transform.h"

namespace physx { class PxRigidActor; }
class FPhysScene_Chaos;
using FPhysScene = FPhysScene_Chaos;
using FChaosScene = FPhysScene_Chaos;
using FPhysicsActorHandle = physx::PxRigidActor*;
struct FActorCreationParams
{
    FChaosScene* Scene = nullptr;
    FTransform InitialTM = FTransform::Identity;
    bool bStatic = false;
    bool bQueryOnly = false;
    bool bEnableGravity = false;
    bool bUpdateKinematicFromSimulation = false;
    bool bSimulatePhysics = false;
    bool bStartAwake = true;
    char* DebugName = nullptr;
};
class ENGINE_API FPhysInterface_Chaos
{
public:
    static void CreateActor(const FActorCreationParams& _InParams, FPhysicsActorHandle& _Handle);
    static void ReleaseActor(FPhysicsActorHandle& _InActorReference, FChaosScene* _InScene = nullptr, bool _bNeverDeferRelease = false);
    static void SetGlobalPose_AssumesLocked(const FPhysicsActorHandle& _InActorReference, const FTransform& _InNewPose, bool _bAutoWake = true);
    static FTransform GetGlobalPose_AssumesLocked(const FPhysicsActorHandle& _InActorReference);
    static void SetLinearVelocity_AssumesLocked(const FPhysicsActorHandle& _InActorReference, const FVector& _InNewVelocity, bool _bAutoWake = true);
    static FVector GetLinearVelocity_AssumesLocked(const FPhysicsActorHandle& _InActorReference);
};
using FPhysicsInterface = FPhysInterface_Chaos;
