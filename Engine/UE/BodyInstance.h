#pragma once
#include "EngineTypes.h"
#include "NameTypes.h"
#include "PhysicsInterface.h"

class UPrimitiveComponent;
class UBodySetup;
struct ENGINE_API FBodyInstance
{
    FBodyInstance() = default;
    ~FBodyInstance();
    FBodyInstance(const FBodyInstance&) = delete;
    FBodyInstance& operator=(const FBodyInstance&) = delete;
    void SetCollisionEnabled(ECollisionEnabled::Type _NewType, bool _bUpdatePhysicsFilterData = true);
    ECollisionEnabled::Type GetCollisionEnabled() const { return __CollisionEnabled; }
    void SetCollisionProfileName(FName _InCollisionProfileName);
    FName GetCollisionProfileName() const { return __CollisionProfileName; }
    void SetObjectType(ECollisionChannel _Channel);
    ECollisionChannel GetObjectType() const { return __ObjectType; }
    bool SetResponseToChannel(ECollisionChannel _Channel, ECollisionResponse _NewResponse);
    bool SetResponseToAllChannels(ECollisionResponse _NewResponse);
    bool SetResponseToChannels(const FCollisionResponseContainer& _NewResponses);
    const FCollisionResponseContainer& GetResponseToChannels() const { return __Responses; }
    ECollisionResponse GetResponseToChannel(ECollisionChannel _Channel) const { return __Responses.GetResponse(_Channel); }
    void SetDOFLock(EDOFMode::Type _NewDOFMode);
    EDOFMode::Type GetDOFLock() const { return __DOFMode; }
    void SetInstanceSimulatePhysics(bool _bSimulate, bool _bMaintainPhysicsBlending = false, bool _bPreserveExistingAttachment = false);
    void SetEnableGravity(bool _bGravityEnabled);
    void SetLinearVelocity(const FVector& _NewVel, bool _bAddToCurrent, bool _bAutoWake = true);
    FVector GetUnrealWorldVelocity() const;
    void SetLinearDamping(float _Damping);
    void SetAngularDamping(float _Damping);
    float GetLinearDamping() const { return __LinearDamping; }
    float GetAngularDamping() const { return __AngularDamping; }
    void AddImpulse(const FVector& _Impulse, bool _bVelChange = false);
    void AddForce(const FVector& _Force, bool _bAllowSubstepping = true, bool _bAccelChange = false);
    void TermBody();
    bool IsValidBodyInstance() const { return __ActorHandle != nullptr; }
    void UpdatePhysicsFilterData();
    void InitBody(UBodySetup* _Setup, const FTransform& _Transform, UPrimitiveComponent* _PrimComp, FPhysScene* _InRBScene);
    bool UpdateBodyScale(const FVector& _InScale3D, bool _bForceUpdate = false);
    bool bSimulatePhysics = false;
    bool bEnableGravity = true;
    bool bNotifyRigidBodyCollision = false;
    bool bUseCCD = false;
    bool bLockXTranslation = false, bLockYTranslation = false, bLockZTranslation = false;
    bool bLockXRotation = false, bLockYRotation = false, bLockZRotation = false;
    FVector CustomDOFPlaneNormal = FVector(0, 1, 0);
private:
    friend class FPhysScene_Chaos;
    friend class UPrimitiveComponent;
    void Recreate();
    FPhysicsActorHandle __ActorHandle = nullptr;
    UPrimitiveComponent* __Owner = nullptr;
    FPhysScene* __Scene = nullptr;
    FName __CollisionProfileName = FName(TEXT("NoCollision"));
    ECollisionEnabled::Type __CollisionEnabled = ECollisionEnabled::NoCollision;
    ECollisionChannel __ObjectType = ECC_WorldStatic;
    FCollisionResponseContainer __Responses;
    EDOFMode::Type __DOFMode = EDOFMode::Default;
    float __LinearDamping = 0;
    float __AngularDamping = 0.05f;
};
