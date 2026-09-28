#include "pch.h"
#include "PhysicsInterface.h"
#include "PhysScene_Chaos.h"
#include "BodyInstance.h"
#include "PrimitiveComponent.h"
#include "Actor.h"
#include "World.h"
#include "CollisionProfile.h"
#pragma push_macro("check")
#undef check
#include <PxPhysicsAPI.h>
#pragma pop_macro("check")
#include <map>
#include <set>
#include <algorithm>
#include <stdexcept>
#include <cmath>

using namespace physx;
namespace
{
    PxVec3 ToPx(const FVector& _V) { return PxVec3(float(_V.X), float(_V.Y), float(_V.Z)); }
    FVector FromPx(const PxVec3& _V) { return FVector(_V.x, _V.y, _V.z); }
    PxQuat ToPx(const FQuat& _Q) { return PxQuat(float(_Q.X), float(_Q.Y), float(_Q.Z), float(_Q.W)); }
    FQuat FromPx(const PxQuat& _Q) { return FQuat(_Q.x, _Q.y, _Q.z, _Q.w); }
    PxTransform ToPx(const FTransform& _T) { return PxTransform(ToPx(_T.GetLocation()), ToPx(_T.GetRotation())); }
    struct FRuntime
    {
        PxDefaultAllocator __Allocator;
        PxDefaultErrorCallback __Errors;
        PxFoundation* __Foundation = nullptr;
        PxPhysics* __Physics = nullptr;
        PxMaterial* __Material = nullptr;
        FRuntime()
        {
            __Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, __Allocator, __Errors);
            if (!__Foundation) throw std::runtime_error("PhysX foundation initialization failed");
            PxTolerancesScale Scale; Scale.length = 100; Scale.speed = 1000;
            __Physics = PxCreatePhysics(PX_PHYSICS_VERSION, *__Foundation, Scale);
            if (!__Physics || !PxInitExtensions(*__Physics, nullptr)) throw std::runtime_error("PhysX initialization failed");
            __Material = __Physics->createMaterial(0.5f, 0.5f, 0.0f);
        }
        ~FRuntime() { __Material->release(); PxCloseExtensions(); __Physics->release(); __Foundation->release(); }
    };
    FRuntime& Runtime() { static FRuntime Instance; return Instance; }
    PxGeometryHolder Geometry(const FCollisionShape& _Shape)
    {
        PxGeometryHolder Result;
        if (_Shape.IsBox()) Result.storeAny(PxBoxGeometry(ToPx(_Shape.GetBox())));
        else if (_Shape.IsSphere()) Result.storeAny(PxSphereGeometry(_Shape.GetSphereRadius()));
        else if (_Shape.IsCapsule()) Result.storeAny(PxCapsuleGeometry(_Shape.GetCapsuleRadius(), (std::max)(0.f, _Shape.GetCapsuleHalfHeight() - _Shape.GetCapsuleRadius())));
        return Result;
    }
    bool ValidShape(const FCollisionShape& _Shape)
    {
        if (_Shape.IsBox()) return !_Shape.GetBox().ContainsNaN() && _Shape.GetBox().GetMin() > 0 && _Shape.GetBox().GetMax() < 1.e15;
        if (_Shape.IsSphere()) return std::isfinite(_Shape.GetSphereRadius()) && _Shape.GetSphereRadius() > 0;
        if (_Shape.IsCapsule()) return std::isfinite(_Shape.GetCapsuleRadius()) && std::isfinite(_Shape.GetCapsuleHalfHeight()) && _Shape.GetCapsuleRadius() > 0 && _Shape.GetCapsuleHalfHeight() >= _Shape.GetCapsuleRadius();
        return false;
    }
    PxTransform ShapePose(const FCollisionShape& _Shape, const FVector& _Position, const FQuat& _Rotation)
    {
        PxQuat Rotation = ToPx(_Rotation);
        if (_Shape.IsCapsule()) Rotation = Rotation * PxQuat(-PxHalfPi, PxVec3(0, 1, 0));
        return PxTransform(ToPx(_Position), Rotation);
    }
    ECollisionResponse PairResponse(const FBodyInstance& _A, const FBodyInstance& _B)
    {
        return (std::min)(_A.GetResponseToChannel(_B.GetObjectType()), _B.GetResponseToChannel(_A.GetObjectType()));
    }
    PxFilterFlags Filter(PxFilterObjectAttributes, PxFilterData _A, PxFilterObjectAttributes, PxFilterData _B, PxPairFlags& _Flags, const void*, PxU32)
    {
        if ((_A.word3 >> 2) != 0 && (_A.word3 >> 2) == (_B.word3 >> 2)) return PxFilterFlag::eSUPPRESS;
        const auto RA = ((_B.word0 < 16 ? _A.word1 : _A.word2) >> ((_B.word0 & 15) * 2)) & 3u;
        const auto RB = ((_A.word0 < 16 ? _B.word1 : _B.word2) >> ((_A.word0 & 15) * 2)) & 3u;
        if ((std::min)(RA, RB) != ECR_Block) return PxFilterFlag::eSUPPRESS;
        _Flags = PxPairFlag::eDETECT_DISCRETE_CONTACT | PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_TOUCH_PERSISTS | PxPairFlag::eNOTIFY_CONTACT_POINTS;
        if (!(_A.word3 & 1) && !(_B.word3 & 1)) _Flags |= PxPairFlag::eSOLVE_CONTACT;
        if ((_A.word3 | _B.word3) & 2) _Flags |= PxPairFlag::eDETECT_CCD_CONTACT;
        return PxFilterFlag::eDEFAULT;
    }
}
const FCollisionQueryParams FCollisionQueryParams::DefaultQueryParam;
const FCollisionResponseParams FCollisionResponseParams::DefaultResponseParam;
AActor* FHitResult::GetActor() const { return Component ? Component->GetOwner() : nullptr; }
AActor* FOverlapResult::GetActor() const { return Component ? Component->GetOwner() : nullptr; }

void FPhysInterface_Chaos::CreateActor(const FActorCreationParams& _InParams, FPhysicsActorHandle& _Handle)
{
    _Handle = nullptr;
    if (_InParams.InitialTM.ContainsNaN()) return;
    auto& Physics = *Runtime().__Physics;
    if (_InParams.bStatic) _Handle = Physics.createRigidStatic(ToPx(_InParams.InitialTM));
    else
    {
        auto* Dynamic = Physics.createRigidDynamic(ToPx(_InParams.InitialTM));
        if (!Dynamic) return;
        Dynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, !_InParams.bSimulatePhysics);
        Dynamic->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !_InParams.bEnableGravity);
        _Handle = Dynamic;
    }
}
void FPhysInterface_Chaos::ReleaseActor(FPhysicsActorHandle& _InActorReference, FChaosScene*, bool)
{
    if (_InActorReference) _InActorReference->release();
    _InActorReference = nullptr;
}
void FPhysInterface_Chaos::SetGlobalPose_AssumesLocked(const FPhysicsActorHandle& _InActorReference, const FTransform& _InNewPose, bool _bAutoWake)
{
    if (_InActorReference && !_InNewPose.ContainsNaN()) _InActorReference->setGlobalPose(ToPx(_InNewPose), _bAutoWake);
}
FTransform FPhysInterface_Chaos::GetGlobalPose_AssumesLocked(const FPhysicsActorHandle& _InActorReference)
{
    if (!_InActorReference) return FTransform::Identity;
    const auto Pose = _InActorReference->getGlobalPose(); return FTransform(FromPx(Pose.q), FromPx(Pose.p));
}
void FPhysInterface_Chaos::SetLinearVelocity_AssumesLocked(const FPhysicsActorHandle& _InActorReference, const FVector& _InNewVelocity, bool _bAutoWake)
{
    if (!_InActorReference || _InNewVelocity.ContainsNaN()) return;
    if (auto* Dynamic = _InActorReference->is<PxRigidDynamic>(); Dynamic && !(Dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC)) Dynamic->setLinearVelocity(ToPx(_InNewVelocity), _bAutoWake);
}
FVector FPhysInterface_Chaos::GetLinearVelocity_AssumesLocked(const FPhysicsActorHandle& _InActorReference)
{
    auto* Dynamic = _InActorReference ? _InActorReference->is<PxRigidDynamic>() : nullptr;
    return Dynamic ? FromPx(Dynamic->getLinearVelocity()) : FVector::ZeroVector;
}

struct FPhysScene_Chaos::FImpl : PxSimulationEventCallback
{
    struct FEntry { PxShape* __Shape = nullptr; PxD6Joint* __Joint = nullptr; FVector __Scale = FVector(1); };
    struct FContact { UPrimitiveComponent* __A; UPrimitiveComponent* __B; FVector __Position, __Normal, __Impulse; };
    struct FOverlapEvent { UPrimitiveComponent* __A; UPrimitiveComponent* __B; bool __Begin; };
    struct FActorOverlapEvent { AActor* __A; AActor* __B; bool __Begin; };
    PxDefaultCpuDispatcher* __Dispatcher = nullptr;
    PxScene* __Scene = nullptr;
    std::map<FBodyInstance*, FEntry> __Bodies;
    std::set<FBodyInstance*> __Registered;
    std::map<AActor*, uint32> __OwnerIds;
    uint32 __NextOwnerId = 1;
    std::set<std::pair<UPrimitiveComponent*, UPrimitiveComponent*>> __Overlaps;
    std::vector<FContact> __Contacts;
    std::vector<FOverlapEvent> __OverlapEvents;
    std::set<std::pair<AActor*, AActor*>> __ActorOverlaps;
    std::vector<FActorOverlapEvent> __ActorOverlapEvents;
    FBodyInstance* __SyncingBody = nullptr;
    bool __Stepping = false;
    double __Accumulator = 0;
    AActor* __SolverActor = nullptr;
    float __FrameDelta = 0;
    double __SubstepDelta = 1.0 / 120.0;
    int32 __MaxSubsteps = 32;
    bool __Substepping = true;
    uint64 __ProfileRevision = UCollisionProfile::Get()->GetRevision();
    FImpl()
    {
        auto& Physics = *Runtime().__Physics;
        __Dispatcher = PxDefaultCpuDispatcherCreate(1);
        PxSceneDesc Description(Physics.getTolerancesScale());
        Description.gravity = PxVec3(0, 0, -980);
        Description.cpuDispatcher = __Dispatcher; Description.filterShader = Filter; Description.simulationEventCallback = this;
        Description.flags |= PxSceneFlag::eENABLE_CCD;
        __Scene = Physics.createScene(Description);
        if (!__Scene) throw std::runtime_error("PhysX scene initialization failed");
    }
    ~FImpl() { __Scene->release(); __Dispatcher->release(); }
    bool Live(UPrimitiveComponent* _Component) const
    {
        for (auto* Body : __Registered) if (Body->__Owner == _Component) return !_Component->IsPendingDestroy() && (!_Component->GetOwner() || !_Component->GetOwner()->IsPendingDestroy());
        return false;
    }
    bool LiveActor(AActor* _Actor) const
    {
        for (auto* Body : __Registered) if (Body->__Owner->GetOwner() == _Actor) return _Actor && !_Actor->IsPendingDestroy();
        return false;
    }
    void onConstraintBreak(PxConstraintInfo*, PxU32) override {}
    void onWake(PxActor**, PxU32) override {}
    void onSleep(PxActor**, PxU32) override {}
    void onTrigger(PxTriggerPair*, PxU32) override {}
    void onAdvance(const PxRigidBody* const*, const PxTransform*, PxU32) override {}
    void onContact(const PxContactPairHeader& _Header, const PxContactPair* _Pairs, PxU32 _Count) override
    {
        if (_Header.flags & (PxContactPairHeaderFlag::eREMOVED_ACTOR_0 | PxContactPairHeaderFlag::eREMOVED_ACTOR_1)) return;
        auto* A = static_cast<FBodyInstance*>(_Header.actors[0]->userData);
        auto* B = static_cast<FBodyInstance*>(_Header.actors[1]->userData);
        if (!A || !B || (!A->bNotifyRigidBodyCollision && !B->bNotifyRigidBodyCollision)) return;
        for (PxU32 Index = 0; Index < _Count; ++Index)
        {
            PxContactPairPoint Point;
            if (_Pairs[Index].extractContacts(&Point, 1)) __Contacts.push_back({A->__Owner, B->__Owner, FromPx(Point.position), FromPx(Point.normal), FromPx(Point.impulse)});
        }
    }
};
FPhysScene_Chaos::FPhysScene_Chaos(AActor* _InSolverActor) : __Impl(std::make_unique<FImpl>()) { __Impl->__SolverActor = _InSolverActor; }
AActor* FPhysScene_Chaos::GetSolverActor() const { return __Impl->__SolverActor; }
void FPhysScene_Chaos::SetUpForFrame(const FVector* _NewGrav, float _InDeltaSeconds, float _InMinPhysicsDeltaTime, float _InMaxPhysicsDeltaTime, float _InMaxSubstepDeltaTime, int32 _InMaxSubsteps, bool _bSubstepping)
{
    if (_NewGrav) SetGravity(*_NewGrav);
    __Impl->__FrameDelta = 0;
    if (!std::isfinite(_InDeltaSeconds) || !std::isfinite(_InMaxPhysicsDeltaTime) || !std::isfinite(_InMaxSubstepDeltaTime) || _InDeltaSeconds < (std::max)(0.f, _InMinPhysicsDeltaTime) || _InMaxPhysicsDeltaTime <= 0 || _InMaxSubstepDeltaTime <= 0 || _InMaxSubsteps <= 0) return;
    __Impl->__Substepping = _bSubstepping; __Impl->__SubstepDelta = _InMaxSubstepDeltaTime; __Impl->__MaxSubsteps = _InMaxSubsteps;
    __Impl->__FrameDelta = (std::min)(_InDeltaSeconds, _InMaxPhysicsDeltaTime);
    if (_bSubstepping) __Impl->__FrameDelta = (std::min)(__Impl->__FrameDelta, _InMaxSubstepDeltaTime * _InMaxSubsteps);
}
void FPhysScene_Chaos::StartFrame() { Tick(__Impl->__FrameDelta); }
void FPhysScene_Chaos::EndFrame() { UpdateOverlaps(); }
FPhysScene_Chaos::~FPhysScene_Chaos()
{
    while (!__Impl->__Registered.empty())
    {
        auto* Body = *__Impl->__Registered.begin(); RemoveBody(Body, true); Body->__Owner = nullptr; Body->__Scene = nullptr;
    }
}
void FPhysScene_Chaos::SetGravity(const FVector& _Gravity) { if (!_Gravity.ContainsNaN()) __Impl->__Scene->setGravity(ToPx(_Gravity)); }
void FPhysScene_Chaos::AddBody(FBodyInstance* _Body)
{
    if (!_Body || !_Body->__Owner) return;
    __Impl->__Registered.insert(_Body);
    if (_Body->__CollisionEnabled == ECollisionEnabled::NoCollision) return;
    const auto Shape = _Body->__Owner->GetCollisionShape();
    if (!ValidShape(Shape)) return;
    const auto Geom = Geometry(Shape);
    FActorCreationParams Params;
    Params.InitialTM = _Body->__Owner->GetComponentTransform(); Params.bStatic = !_Body->bSimulatePhysics && _Body->GetObjectType() == ECC_WorldStatic;
    Params.bSimulatePhysics = _Body->bSimulatePhysics; Params.bEnableGravity = _Body->bEnableGravity; Params.Scene = this;
    FPhysicsInterface::CreateActor(Params, _Body->__ActorHandle);
    if (!_Body->__ActorHandle) return;
    PxShape* NativeShape = Runtime().__Physics->createShape(Geom.any(), *Runtime().__Material, true);
    if (!NativeShape) { FPhysicsInterface::ReleaseActor(_Body->__ActorHandle); return; }
    NativeShape->userData = _Body->__Owner;
    if (Shape.IsCapsule()) NativeShape->setLocalPose(PxTransform(PxQuat(-PxHalfPi, PxVec3(0, 1, 0))));
    NativeShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, CollisionHasPhysics(_Body->__CollisionEnabled));
    NativeShape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, CollisionHasQuery(_Body->__CollisionEnabled));
    PxFilterData FilterData; FilterData.word0 = _Body->GetObjectType();
    for (int Index = 0; Index < 32; ++Index)
    {
        auto& Word = Index < 16 ? FilterData.word1 : FilterData.word2;
        Word |= uint32(_Body->GetResponseToChannel(static_cast<ECollisionChannel>(Index))) << ((Index & 15) * 2);
    }
    FilterData.word3 = ((_Body->__CollisionEnabled == ECollisionEnabled::ProbeOnly || _Body->__CollisionEnabled == ECollisionEnabled::QueryAndProbe) ? 1u : 0u) | (_Body->bUseCCD ? 2u : 0u);
    if (auto* Owner = _Body->__Owner->GetOwner())
    {
        auto [Found, Inserted] = __Impl->__OwnerIds.try_emplace(Owner, __Impl->__NextOwnerId);
        if (Inserted) ++__Impl->__NextOwnerId;
        FilterData.word3 |= Found->second << 2;
    }
    NativeShape->setSimulationFilterData(FilterData);
    _Body->__ActorHandle->attachShape(*NativeShape); NativeShape->release();
    _Body->__ActorHandle->userData = _Body;
    auto& Entry = __Impl->__Bodies[_Body]; Entry.__Shape = NativeShape; Entry.__Scale = Params.InitialTM.GetScale3D();
    if (auto* Dynamic = _Body->__ActorHandle->is<PxRigidDynamic>())
    {
        PxRigidBodyExt::setMassAndUpdateInertia(*Dynamic, 1.0f);
        Dynamic->setLinearDamping(_Body->__LinearDamping);
        Dynamic->setAngularDamping(_Body->__AngularDamping);
        Dynamic->setRigidBodyFlag(PxRigidBodyFlag::eRETAIN_ACCELERATIONS, true);
        Dynamic->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, _Body->bUseCCD && _Body->bSimulatePhysics);
        PxRigidDynamicLockFlags Locks;
        const auto Mode = _Body->__DOFMode;
        if (Mode == EDOFMode::YZPlane) Locks = PxRigidDynamicLockFlag::eLOCK_LINEAR_X | PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y | PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z;
        if (Mode == EDOFMode::XZPlane) Locks = PxRigidDynamicLockFlag::eLOCK_LINEAR_Y | PxRigidDynamicLockFlag::eLOCK_ANGULAR_X | PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z;
        if (Mode == EDOFMode::XYPlane) Locks = PxRigidDynamicLockFlag::eLOCK_LINEAR_Z | PxRigidDynamicLockFlag::eLOCK_ANGULAR_X | PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y;
        if (Mode == EDOFMode::SixDOF)
        {
            if (_Body->bLockXTranslation) Locks |= PxRigidDynamicLockFlag::eLOCK_LINEAR_X;
            if (_Body->bLockYTranslation) Locks |= PxRigidDynamicLockFlag::eLOCK_LINEAR_Y;
            if (_Body->bLockZTranslation) Locks |= PxRigidDynamicLockFlag::eLOCK_LINEAR_Z;
            if (_Body->bLockXRotation) Locks |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_X;
            if (_Body->bLockYRotation) Locks |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y;
            if (_Body->bLockZRotation) Locks |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z;
        }
        Dynamic->setRigidDynamicLockFlags(Locks);
        if (Mode == EDOFMode::CustomPlane && _Body->bSimulatePhysics)
        {
            FVector Normal = _Body->CustomDOFPlaneNormal.GetSafeNormal();
            if (Normal.IsNearlyZero()) Normal = FVector(0, 1, 0);
            const FQuat Rotation = FQuat::FindBetweenNormals(FVector(1, 0, 0), Normal);
            PxTransform WorldFrame(ToPx(Params.InitialTM.GetLocation()), ToPx(Rotation));
            Entry.__Joint = PxD6JointCreate(*Runtime().__Physics, nullptr, WorldFrame, Dynamic, Dynamic->getGlobalPose().getInverse() * WorldFrame);
            Entry.__Joint->setMotion(PxD6Axis::eY, PxD6Motion::eFREE); Entry.__Joint->setMotion(PxD6Axis::eZ, PxD6Motion::eFREE); Entry.__Joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);
        }
    }
    __Impl->__Scene->addActor(*_Body->__ActorHandle);
}
void FPhysScene_Chaos::RemoveBody(FBodyInstance* _Body, bool _bUnregister)
{
    if (_bUnregister)
    {
        __Impl->__Registered.erase(_Body);
        auto* Owner = _Body->__Owner;
        std::erase_if(__Impl->__Overlaps, [&](const auto& _Pair) { return _Pair.first == Owner || _Pair.second == Owner; });
        std::erase_if(__Impl->__OverlapEvents, [&](const auto& _Event) { return _Event.__A == Owner || _Event.__B == Owner; });
        std::erase_if(__Impl->__Contacts, [&](const auto& _Event) { return _Event.__A == Owner || _Event.__B == Owner; });
        auto* Actor = Owner ? Owner->GetOwner() : nullptr;
        if (!__Impl->LiveActor(Actor))
        {
            std::erase_if(__Impl->__ActorOverlaps, [&](const auto& _Pair) { return _Pair.first == Actor || _Pair.second == Actor; });
            std::erase_if(__Impl->__ActorOverlapEvents, [&](const auto& _Event) { return _Event.__A == Actor || _Event.__B == Actor; });
        }
    }
    auto Found = __Impl->__Bodies.find(_Body);
    if (Found != __Impl->__Bodies.end())
    {
        if (Found->second.__Joint) Found->second.__Joint->release();
        __Impl->__Bodies.erase(Found);
    }
    FPhysicsInterface::ReleaseActor(_Body->__ActorHandle);
}
void FPhysScene_Chaos::UpdateTransform(FBodyInstance* _Body)
{
    if (__Impl->__SyncingBody == _Body || !_Body->__Owner || !_Body->__ActorHandle) return;
    const auto Found = __Impl->__Bodies.find(_Body);
    if (Found != __Impl->__Bodies.end() && !Found->second.__Scale.Equals(_Body->__Owner->GetComponentScale())) { _Body->Recreate(); return; }
    FPhysicsInterface::SetGlobalPose_AssumesLocked(_Body->__ActorHandle, _Body->__Owner->GetComponentTransform());
}
void FPhysScene_Chaos::UpdateOverlaps()
{
    std::set<std::pair<UPrimitiveComponent*, UPrimitiveComponent*>> Current;
    for (auto A = __Impl->__Bodies.begin(); A != __Impl->__Bodies.end(); ++A)
    {
        auto* CA = A->first->__Owner;
        if (!CA->GetGenerateOverlapEvents() || !CollisionHasQuery(CA->GetCollisionEnabled()) || !__Impl->Live(CA)) continue;
        for (auto B = std::next(A); B != __Impl->__Bodies.end(); ++B)
        {
            auto* CB = B->first->__Owner;
            if (!CB->GetGenerateOverlapEvents() || !CollisionHasQuery(CB->GetCollisionEnabled()) || !__Impl->Live(CB) || (CA->GetOwner() && CA->GetOwner() == CB->GetOwner()) || PairResponse(*A->first, *B->first) != ECR_Overlap) continue;
            if (PxGeometryQuery::overlap(A->second.__Shape->getGeometry(), A->first->__ActorHandle->getGlobalPose() * A->second.__Shape->getLocalPose(), B->second.__Shape->getGeometry(), B->first->__ActorHandle->getGlobalPose() * B->second.__Shape->getLocalPose()))
                Current.insert(std::less<UPrimitiveComponent*>()(CA, CB) ? std::make_pair(CA, CB) : std::make_pair(CB, CA));
        }
    }
    for (const auto& Pair : Current) if (!__Impl->__Overlaps.count(Pair)) __Impl->__OverlapEvents.push_back({Pair.first, Pair.second, true});
    for (const auto& Pair : __Impl->__Overlaps) if (!Current.count(Pair)) __Impl->__OverlapEvents.push_back({Pair.first, Pair.second, false});
    std::set<std::pair<AActor*, AActor*>> CurrentActors;
    for (const auto& Pair : Current)
    {
        auto* A = Pair.first->GetOwner(); auto* B = Pair.second->GetOwner();
        if (A && B) CurrentActors.insert(std::less<AActor*>()(A, B) ? std::make_pair(A, B) : std::make_pair(B, A));
    }
    for (const auto& Pair : CurrentActors) if (!__Impl->__ActorOverlaps.count(Pair)) __Impl->__ActorOverlapEvents.push_back({Pair.first, Pair.second, true});
    for (const auto& Pair : __Impl->__ActorOverlaps) if (!CurrentActors.count(Pair)) __Impl->__ActorOverlapEvents.push_back({Pair.first, Pair.second, false});
    __Impl->__ActorOverlaps = std::move(CurrentActors);
    __Impl->__Overlaps = std::move(Current);
    if (!__Impl->__Stepping) DispatchEvents();
}
void FPhysScene_Chaos::Tick(float _DeltaTime)
{
    if (!std::isfinite(_DeltaTime) || _DeltaTime < 0) return;
    __Impl->__Stepping = true;
    if (__Impl->__ProfileRevision != UCollisionProfile::Get()->GetRevision())
    {
        __Impl->__ProfileRevision = UCollisionProfile::Get()->GetRevision();
        std::vector<FBodyInstance*> Bodies;
        for (auto* Body : __Impl->__Registered) Bodies.push_back(Body);
        for (auto* Body : Bodies) if (Body->__CollisionProfileName != FName(TEXT("Custom"))) Body->SetCollisionProfileName(Body->__CollisionProfileName);
    }
    __Impl->__Accumulator += _DeltaTime;
    const double Step = __Impl->__Substepping ? __Impl->__SubstepDelta : __Impl->__Accumulator;
    bool Stepped = false;
    int32 Substeps = 0;
    while (Step > 0 && __Impl->__Accumulator + 1.e-9 >= Step && Substeps++ < __Impl->__MaxSubsteps)
    {
        __Impl->__Scene->simulate(float(Step)); __Impl->__Scene->fetchResults(true); __Impl->__Accumulator -= Step;
        Stepped = true;
        for (const auto& [Body, Entry] : __Impl->__Bodies)
        {
            if (!Body->bSimulatePhysics) continue;
            __Impl->__SyncingBody = Body;
            const auto Pose = FPhysicsInterface::GetGlobalPose_AssumesLocked(Body->__ActorHandle);
            Body->__Owner->SetWorldRotation(Pose.GetRotation()); Body->__Owner->SetWorldLocation(Pose.GetLocation());
        }
        __Impl->__SyncingBody = nullptr;
        UpdateOverlaps();
    }
    UpdateOverlaps();
    if (Stepped) for (const auto& [Body, Entry] : __Impl->__Bodies)
        if (Body->bSimulatePhysics) if (auto* Dynamic = Body->__ActorHandle->is<PxRigidDynamic>()) { Dynamic->clearForce(); Dynamic->clearTorque(); Dynamic->clearForce(PxForceMode::eACCELERATION); Dynamic->clearTorque(PxForceMode::eACCELERATION); }
    __Impl->__Stepping = false;
}
void FPhysScene_Chaos::DispatchEvents()
{
    UWorld* World = __Impl->__Registered.empty() ? nullptr : (*__Impl->__Registered.begin())->__Owner->GetWorld();
    std::unique_lock<std::recursive_mutex> Lock;
    bool WasTicking = false;
    if (World) { Lock = std::unique_lock<std::recursive_mutex>(World->__WorldMutex); WasTicking = World->__bIsTicking; World->__bIsTicking = true; }
    auto Events = std::move(__Impl->__OverlapEvents); __Impl->__OverlapEvents.clear();
    for (const auto& Event : Events)
    {
        auto* A = Event.__A; auto* B = Event.__B;
        if (!__Impl->Live(A) || !__Impl->Live(B)) continue;
        if (Event.__Begin)
        {
            FHitResult Hit; Hit.Component = B;
            A->OnComponentBeginOverlap.Broadcast(A, B->GetOwner(), B, 0, false, Hit);
            if (__Impl->Live(A) && __Impl->Live(B)) { Hit.Component = A; B->OnComponentBeginOverlap.Broadcast(B, A->GetOwner(), A, 0, false, Hit); }
        }
        else
        {
            A->OnComponentEndOverlap.Broadcast(A, B->GetOwner(), B, 0);
            if (__Impl->Live(A) && __Impl->Live(B)) B->OnComponentEndOverlap.Broadcast(B, A->GetOwner(), A, 0);
        }
    }
    auto ActorEvents = std::move(__Impl->__ActorOverlapEvents); __Impl->__ActorOverlapEvents.clear();
    for (const auto& Event : ActorEvents)
    {
        auto Notify = [&](AActor* _Self, AActor* _Other)
        {
            if (!__Impl->LiveActor(_Self) || !__Impl->LiveActor(_Other)) return;
            if (Event.__Begin) _Self->NotifyActorBeginOverlap(_Other); else _Self->NotifyActorEndOverlap(_Other);
            if (!__Impl->LiveActor(_Self) || !__Impl->LiveActor(_Other)) return;
            if (Event.__Begin) _Self->OnActorBeginOverlap.Broadcast(_Self, _Other); else _Self->OnActorEndOverlap.Broadcast(_Self, _Other);
        };
        Notify(Event.__A, Event.__B); Notify(Event.__B, Event.__A);
    }
    auto Contacts = std::move(__Impl->__Contacts); __Impl->__Contacts.clear();
    for (const auto& Contact : Contacts)
    {
        auto* A = Contact.__A; auto* B = Contact.__B;
        if (!__Impl->Live(A) || !__Impl->Live(B)) continue;
        FHitResult Hit; Hit.bBlockingHit = true; Hit.Component = B; Hit.Location = Hit.ImpactPoint = Contact.__Position; Hit.Normal = Hit.ImpactNormal = Contact.__Normal;
        auto NotifyHit = [&](UPrimitiveComponent* _Self, UPrimitiveComponent* _Other, const FVector& _Impulse)
        {
            if (!__Impl->Live(_Self) || !__Impl->Live(_Other)) return;
            _Self->OnComponentHit.Broadcast(_Self, _Other->GetOwner(), _Other, _Impulse, Hit);
            if (!__Impl->Live(_Self) || !__Impl->Live(_Other)) return;
            auto* Actor = _Self->GetOwner();
            if (!Actor) return;
            Actor->NotifyHit(_Self, _Other->GetOwner(), _Other, _Self->IsSimulatingPhysics(), Hit.ImpactPoint, Hit.ImpactNormal, _Impulse, Hit);
            if (__Impl->Live(_Self) && __Impl->Live(_Other)) Actor->OnActorHit.Broadcast(Actor, _Other->GetOwner(), _Impulse, Hit);
        };
        if (A->BodyInstance.bNotifyRigidBodyCollision) NotifyHit(A, B, Contact.__Impulse);
        if (__Impl->Live(A) && __Impl->Live(B) && B->BodyInstance.bNotifyRigidBodyCollision)
        {
            Hit.Component = A; Hit.Normal = Hit.ImpactNormal = -Contact.__Normal; NotifyHit(B, A, -Contact.__Impulse);
        }
    }
    if (World) { World->__bIsTicking = WasTicking; if (!WasTicking) { World->FlushPendingDestroyComponents(); World->FlushPendingDestroyActors(); } }
}

namespace
{
    class FQueryFilter : public PxQueryFilterCallback
    {
    public:
        FQueryFilter(ECollisionChannel _Channel, const FCollisionQueryParams& _Params, const FCollisionResponseParams& _Response, const FCollisionObjectQueryParams* _Objects, bool _Overlap)
            : __Channel(_Channel), __Params(_Params), __Response(_Response), __Objects(_Objects), __Overlap(_Overlap) {}
        ECollisionResponse Response(const UPrimitiveComponent* _Component) const
        {
            if (__Objects) return __Objects->Contains(_Component->GetCollisionObjectType()) ? ECR_Block : ECR_Ignore;
            return (std::min)(_Component->GetCollisionResponseToChannel(__Channel), __Response.CollisionResponse.GetResponse(_Component->GetCollisionObjectType()));
        }
        PxQueryHitType::Enum preFilter(const PxFilterData&, const PxShape* _Shape, const PxRigidActor*, PxHitFlags&) override
        {
            auto* Component = static_cast<UPrimitiveComponent*>(_Shape->userData);
            if (!Component || Component->IsPendingDestroy() || (Component->GetOwner() && Component->GetOwner()->IsPendingDestroy()) || __Params.IsIgnored(Component->GetOwner(), Component)) return PxQueryHitType::eNONE;
            const auto HitResponse = Response(Component);
            if (HitResponse == ECR_Ignore || (HitResponse == ECR_Block && __Params.bIgnoreBlocks) || (HitResponse == ECR_Overlap && __Params.bIgnoreTouches)) return PxQueryHitType::eNONE;
            return PxQueryHitType::eTOUCH;
        }
        PxQueryHitType::Enum postFilter(const PxFilterData& _Data, const PxQueryHit& _Hit, const PxShape* _Shape, const PxRigidActor* _Actor) override
        {
            if (!__Overlap && !__Params.bFindInitialOverlaps && static_cast<const PxLocationHit&>(_Hit).hadInitialOverlap()) return PxQueryHitType::eNONE;
            PxHitFlags Flags; return preFilter(_Data, _Shape, _Actor, Flags);
        }
    private:
        ECollisionChannel __Channel;
        const FCollisionQueryParams& __Params;
        const FCollisionResponseParams& __Response;
        const FCollisionObjectQueryParams* __Objects;
        bool __Overlap;
    };
    void FillHit(FHitResult& _Out, const PxLocationHit& _Hit, PxShape* _Shape, const FVector& _Start, const FVector& _End, bool _Sweep)
    {
        _Out.Distance = (std::max)(0.f, _Hit.distance); _Out.TraceStart = _Start; _Out.TraceEnd = _End;
        const double Distance = (_End - _Start).Size(); _Out.Time = Distance > 0 ? float(_Out.Distance / Distance) : 0;
        _Out.Location = _Start + (_End - _Start) * _Out.Time;
        _Out.ImpactPoint = (_Hit.flags & PxHitFlag::ePOSITION) ? FromPx(_Hit.position) : _Out.Location;
        _Out.Normal = _Out.ImpactNormal = (_Hit.flags & PxHitFlag::eNORMAL) ? FromPx(_Hit.normal) : FVector::ZeroVector;
        _Out.Component = static_cast<UPrimitiveComponent*>(_Shape->userData);
        _Out.bStartPenetrating = _Sweep && _Hit.hadInitialOverlap();
        if (_Out.bStartPenetrating) { _Out.PenetrationDepth = (std::max)(0.f, -_Hit.distance); _Out.Location = _Start; }
    }
}
bool FPhysScene_Chaos::TraceMulti(std::vector<FHitResult>& _OutHits, const FVector& _Start, const FVector& _End, const FQuat& _Rot, const FCollisionShape* _Shape, ECollisionChannel _Channel, const FCollisionQueryParams& _Params, const FCollisionResponseParams& _Response, const FCollisionObjectQueryParams* _Objects) const
{
    _OutHits.clear();
    if (_Params.bTraceComplex || _Start.ContainsNaN() || _End.ContainsNaN() || (!_Objects && (_Channel < 0 || _Channel >= 32)) || (_Objects && !_Objects->IsValid())) return false;
    if (_Shape && (!ValidShape(*_Shape) || _Rot.ContainsNaN() || !_Rot.IsNormalized())) return false;
    const FVector Delta = _End - _Start;
    const float Length = float(Delta.Size());
    if (!std::isfinite(Length) || (!_Shape && Length < 1.e-6f)) return false;
    const PxVec3 Direction = Length > 1.e-6f ? ToPx(Delta / Length) : PxVec3(1, 0, 0);
    FQueryFilter Callback(_Channel, _Params, _Response, _Objects, false);
    PxQueryFilterData FilterData(PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER | PxQueryFlag::ePOSTFILTER);
    auto Append = [&](const auto& _Buffer)
    {
        for (PxU32 Index = 0; Index < _Buffer.getNbTouches(); ++Index)
        {
            const auto& NativeHit = _Buffer.getTouch(Index);
            FHitResult Hit;
            FillHit(Hit, NativeHit, NativeHit.shape, _Start, _End, _Shape != nullptr);
            Hit.bBlockingHit = Callback.Response(Hit.Component) == ECR_Block;
            _OutHits.push_back(Hit);
        }
    };
    if (_Shape)
    {
        std::vector<PxSweepHit> Storage(__Impl->__Bodies.size() + 1);
        PxSweepBuffer Buffer(Storage.data(), PxU32(Storage.size()));
        const auto Geom = Geometry(*_Shape);
        __Impl->__Scene->sweep(Geom.any(), ShapePose(*_Shape, _Start, _Rot), Direction, Length, Buffer, PxHitFlag::eDEFAULT | PxHitFlag::eMTD, FilterData, &Callback);
        Append(Buffer);
    }
    else
    {
        std::vector<PxRaycastHit> Storage(__Impl->__Bodies.size() + 1);
        PxRaycastBuffer Buffer(Storage.data(), PxU32(Storage.size()));
        __Impl->__Scene->raycast(ToPx(_Start), Direction, Length, Buffer, PxHitFlag::eDEFAULT, FilterData, &Callback);
        Append(Buffer);
    }
    std::stable_sort(_OutHits.begin(), _OutHits.end(), [](const auto& _A, const auto& _B)
    {
        if (_A.Distance != _B.Distance) return _A.Distance < _B.Distance;
        return !_A.bBlockingHit && _B.bBlockingHit;
    });
    const auto Block = std::find_if(_OutHits.begin(), _OutHits.end(), [](const auto& _Hit) { return _Hit.bBlockingHit; });
    const bool Blocking = Block != _OutHits.end();
    if (!_Objects && Blocking) _OutHits.erase(std::next(Block), _OutHits.end());
    return _Objects ? !_OutHits.empty() : Blocking;
}
bool FPhysScene_Chaos::OverlapMulti(std::vector<FOverlapResult>& _OutOverlaps, const FVector& _Pos, const FQuat& _Rot, const FCollisionShape& _Shape, ECollisionChannel _Channel, const FCollisionQueryParams& _Params, const FCollisionResponseParams& _Response, const FCollisionObjectQueryParams* _Objects) const
{
    _OutOverlaps.clear();
    if (_Params.bTraceComplex || !ValidShape(_Shape) || _Pos.ContainsNaN() || _Rot.ContainsNaN() || !_Rot.IsNormalized() || (!_Objects && (_Channel < 0 || _Channel >= 32)) || (_Objects && !_Objects->IsValid())) return false;
    FQueryFilter Callback(_Channel, _Params, _Response, _Objects, true);
    std::vector<PxOverlapHit> Storage(__Impl->__Bodies.size() + 1);
    PxOverlapBuffer Buffer(Storage.data(), PxU32(Storage.size()));
    const auto Geom = Geometry(_Shape);
    PxQueryFilterData FilterData(PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER);
    __Impl->__Scene->overlap(Geom.any(), ShapePose(_Shape, _Pos, _Rot), Buffer, FilterData, &Callback);
    bool Blocking = false;
    for (PxU32 Index = 0; Index < Buffer.getNbTouches(); ++Index)
    {
        auto* Component = static_cast<UPrimitiveComponent*>(Buffer.getTouch(Index).shape->userData);
        const bool Block = Callback.Response(Component) == ECR_Block;
        _OutOverlaps.push_back({Block, Component}); Blocking |= Block;
    }
    return _Objects ? !_OutOverlaps.empty() : Blocking;
}
bool FPhysScene_Chaos::LineTraceSingleByChannel(FHitResult& _OutHit, const FVector& _Start, const FVector& _End, ECollisionChannel _TraceChannel, const FCollisionQueryParams& _Params, const FCollisionResponseParams& _ResponseParam) const
{
    _OutHit = {}; _OutHit.TraceStart = _Start; _OutHit.TraceEnd = _End;
    auto Params = _Params; Params.bIgnoreTouches = true;
    std::vector<FHitResult> Hits;
    if (!TraceMulti(Hits, _Start, _End, FQuat::Identity, nullptr, _TraceChannel, Params, _ResponseParam)) return false;
    _OutHit = Hits.front(); return true;
}
bool FPhysScene_Chaos::SweepSingleByChannel(FHitResult& _OutHit, const FVector& _Start, const FVector& _End, const FQuat& _Rot, ECollisionChannel _TraceChannel, const FCollisionShape& _Shape, const FCollisionQueryParams& _Params, const FCollisionResponseParams& _ResponseParam) const
{
    _OutHit = {}; _OutHit.TraceStart = _Start; _OutHit.TraceEnd = _End;
    auto Params = _Params; Params.bIgnoreTouches = true;
    std::vector<FHitResult> Hits;
    if (!TraceMulti(Hits, _Start, _End, _Rot, &_Shape, _TraceChannel, Params, _ResponseParam)) return false;
    _OutHit = Hits.front(); return true;
}
bool FPhysScene_Chaos::OverlapMultiByChannel(std::vector<FOverlapResult>& _OutOverlaps, const FVector& _Pos, const FQuat& _Rot, ECollisionChannel _TraceChannel, const FCollisionShape& _Shape, const FCollisionQueryParams& _Params, const FCollisionResponseParams& _ResponseParam) const
{
    return OverlapMulti(_OutOverlaps, _Pos, _Rot, _Shape, _TraceChannel, _Params, _ResponseParam);
}
FBodyInstance::~FBodyInstance() { TermBody(); }
void FBodyInstance::InitBody(UBodySetup*, const FTransform& _Transform, UPrimitiveComponent* _PrimComp, FPhysScene* _InRBScene)
{
    TermBody(); __Owner = _PrimComp; __Scene = _InRBScene;
    if (__Scene) __Scene->AddBody(this);
    FPhysicsInterface::SetGlobalPose_AssumesLocked(__ActorHandle, _Transform);
}
void FBodyInstance::TermBody()
{
    if (__Scene) __Scene->RemoveBody(this, true);
    __Scene = nullptr; __Owner = nullptr;
}
void FBodyInstance::Recreate()
{
    if (!__Scene) return;
    const auto Velocity = GetUnrealWorldVelocity();
    const auto* Dynamic = __ActorHandle ? __ActorHandle->is<PxRigidDynamic>() : nullptr;
    const PxVec3 AngularVelocity = Dynamic ? Dynamic->getAngularVelocity() : PxVec3(0);
    __Scene->RemoveBody(this); __Scene->AddBody(this); SetLinearVelocity(Velocity, false);
    if (bSimulatePhysics && __ActorHandle) if (auto* NewDynamic = __ActorHandle->is<PxRigidDynamic>()) NewDynamic->setAngularVelocity(AngularVelocity);
}
void FBodyInstance::UpdatePhysicsFilterData() { Recreate(); }
bool FBodyInstance::UpdateBodyScale(const FVector& _InScale3D, bool)
{
    if (_InScale3D.ContainsNaN()) return false;
    Recreate(); return IsValidBodyInstance();
}
void FBodyInstance::SetCollisionEnabled(ECollisionEnabled::Type _NewType, bool _bUpdatePhysicsFilterData)
{
    if (_NewType < 0 || _NewType > ECollisionEnabled::QueryAndProbe) return;
    __CollisionEnabled = _NewType; __CollisionProfileName = FName(TEXT("Custom")); if (_bUpdatePhysicsFilterData) Recreate();
}
void FBodyInstance::SetCollisionProfileName(FName _InCollisionProfileName)
{
    if (_InCollisionProfileName == FName(TEXT("Custom"))) { __CollisionProfileName = _InCollisionProfileName; return; }
    FCollisionResponseTemplate Profile;
    if (!UCollisionProfile::Get()->GetProfileTemplate(_InCollisionProfileName, Profile)) return;
    __CollisionProfileName = Profile.Name; __CollisionEnabled = Profile.CollisionEnabled; __ObjectType = Profile.ObjectType; __Responses = Profile.ResponseToChannels; Recreate();
}
void FBodyInstance::SetObjectType(ECollisionChannel _Channel)
{
    if (_Channel < 0 || _Channel >= 32) return;
    __ObjectType = _Channel; __CollisionProfileName = FName(TEXT("Custom")); Recreate();
}
bool FBodyInstance::SetResponseToChannel(ECollisionChannel _Channel, ECollisionResponse _NewResponse)
{
    if (!__Responses.SetResponse(_Channel, _NewResponse)) return false;
    __CollisionProfileName = FName(TEXT("Custom")); Recreate(); return true;
}
bool FBodyInstance::SetResponseToAllChannels(ECollisionResponse _NewResponse)
{
    if (_NewResponse < 0 || _NewResponse >= ECR_MAX) return false;
    __Responses.SetAllChannels(_NewResponse); __CollisionProfileName = FName(TEXT("Custom")); Recreate(); return true;
}
bool FBodyInstance::SetResponseToChannels(const FCollisionResponseContainer& _NewResponses)
{
    __Responses = _NewResponses; __CollisionProfileName = FName(TEXT("Custom")); Recreate(); return true;
}
void FBodyInstance::SetDOFLock(EDOFMode::Type _NewDOFMode) { if (_NewDOFMode >= 0 && _NewDOFMode <= EDOFMode::None) { __DOFMode = _NewDOFMode; Recreate(); } }
void FBodyInstance::SetInstanceSimulatePhysics(bool _bSimulate, bool, bool _bPreserveExistingAttachment)
{
    if (_bSimulate && !_bPreserveExistingAttachment && __Owner) __Owner->DetachFromComponent();
    bSimulatePhysics = _bSimulate; Recreate();
}
void FBodyInstance::SetEnableGravity(bool _bGravityEnabled)
{
    bEnableGravity = _bGravityEnabled; if (__ActorHandle) __ActorHandle->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !_bGravityEnabled);
}
void FBodyInstance::SetLinearVelocity(const FVector& _NewVel, bool _bAddToCurrent, bool _bAutoWake)
{
    FPhysicsInterface::SetLinearVelocity_AssumesLocked(__ActorHandle, _bAddToCurrent ? GetUnrealWorldVelocity() + _NewVel : _NewVel, _bAutoWake);
}
FVector FBodyInstance::GetUnrealWorldVelocity() const { return FPhysicsInterface::GetLinearVelocity_AssumesLocked(__ActorHandle); }
void FBodyInstance::SetLinearDamping(float _Damping)
{
    if (!std::isfinite(_Damping) || _Damping < 0) return;
    __LinearDamping = _Damping;
    if (auto* Dynamic = __ActorHandle ? __ActorHandle->is<PxRigidDynamic>() : nullptr) Dynamic->setLinearDamping(_Damping);
}
void FBodyInstance::SetAngularDamping(float _Damping)
{
    if (!std::isfinite(_Damping) || _Damping < 0) return;
    __AngularDamping = _Damping;
    if (auto* Dynamic = __ActorHandle ? __ActorHandle->is<PxRigidDynamic>() : nullptr) Dynamic->setAngularDamping(_Damping);
}
void FBodyInstance::AddImpulse(const FVector& _Impulse, bool _bVelChange)
{
    if (_Impulse.ContainsNaN() || !bSimulatePhysics || !__ActorHandle) return;
    if (auto* Dynamic = __ActorHandle->is<PxRigidDynamic>()) Dynamic->addForce(ToPx(_Impulse), _bVelChange ? PxForceMode::eVELOCITY_CHANGE : PxForceMode::eIMPULSE);
}
void FBodyInstance::AddForce(const FVector& _Force, bool, bool _bAccelChange)
{
    if (_Force.ContainsNaN() || !bSimulatePhysics || !__ActorHandle) return;
    if (auto* Dynamic = __ActorHandle->is<PxRigidDynamic>()) Dynamic->addForce(ToPx(_Force), _bAccelChange ? PxForceMode::eACCELERATION : PxForceMode::eFORCE);
}
