#pragma once

#include "SceneComponent.h"
#include "BodyInstance.h"
#include "CollisionShape.h"
#include "CollisionQueryParams.h"
#include "MulticastDelegate.h"

class FPrimitiveSceneProxy;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS(Abstract, MinimalAPI)
class ENGINE_API UPrimitiveComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UPrimitiveComponent() = default;
    static UClass* StaticClass() { return UClass::For<UPrimitiveComponent>(); }
	FBodyInstance BodyInstance;
	virtual FCollisionShape GetCollisionShape(float _Inflation = 0.0f) const { return {}; }
	virtual FBodyInstance* GetBodyInstance(FName _BoneName = NAME_None, bool _bGetWelded = true, int32 _Index = -1) const { return const_cast<FBodyInstance*>(&BodyInstance); }
	virtual void SetCollisionEnabled(ECollisionEnabled::Type _NewType) { BodyInstance.SetCollisionEnabled(_NewType); }
	ECollisionEnabled::Type GetCollisionEnabled() const { return BodyInstance.GetCollisionEnabled(); }
	virtual void SetCollisionProfileName(FName _InCollisionProfileName, bool _bUpdateOverlaps = true);
	FName GetCollisionProfileName() const { return BodyInstance.GetCollisionProfileName(); }
	void SetCollisionObjectType(ECollisionChannel _Channel) { BodyInstance.SetObjectType(_Channel); }
	ECollisionChannel GetCollisionObjectType() const { return BodyInstance.GetObjectType(); }
	void SetCollisionResponseToChannel(ECollisionChannel _Channel, ECollisionResponse _NewResponse) { BodyInstance.SetResponseToChannel(_Channel, _NewResponse); }
	void SetCollisionResponseToAllChannels(ECollisionResponse _NewResponse) { BodyInstance.SetResponseToAllChannels(_NewResponse); }
	void SetCollisionResponseToChannels(const FCollisionResponseContainer& _NewResponses) { BodyInstance.SetResponseToChannels(_NewResponses); }
	ECollisionResponse GetCollisionResponseToChannel(ECollisionChannel _Channel) const { return BodyInstance.GetResponseToChannel(_Channel); }
	void SetSimulatePhysics(bool _bSimulate) { BodyInstance.SetInstanceSimulatePhysics(_bSimulate); }
	bool IsSimulatingPhysics(FName _BoneName = NAME_None) const { return BodyInstance.bSimulatePhysics; }
	void SetEnableGravity(bool _bGravityEnabled) { BodyInstance.SetEnableGravity(_bGravityEnabled); }
	bool IsGravityEnabled() const { return BodyInstance.bEnableGravity; }
	void SetNotifyRigidBodyCollision(bool _bNewNotifyRigidBodyCollision) { BodyInstance.bNotifyRigidBodyCollision = _bNewNotifyRigidBodyCollision; }
	void SetGenerateOverlapEvents(bool _bInGenerateOverlapEvents) { __GenerateOverlapEvents = _bInGenerateOverlapEvents; }
	bool GetGenerateOverlapEvents() const { return __GenerateOverlapEvents; }
	void SetPhysicsLinearVelocity(FVector _NewVel, bool _bAddToCurrent = false, FName _BoneName = NAME_None) { BodyInstance.SetLinearVelocity(_NewVel, _bAddToCurrent); }
	FVector GetPhysicsLinearVelocity(FName _BoneName = NAME_None) const { return BodyInstance.GetUnrealWorldVelocity(); }
	void AddImpulse(FVector _Impulse, FName _BoneName = NAME_None, bool _bVelChange = false) { BodyInstance.AddImpulse(_Impulse, _bVelChange); }
    virtual void SetLinearDamping(float _InDamping) { BodyInstance.SetLinearDamping(_InDamping); }
    virtual void SetAngularDamping(float _InDamping) { BodyInstance.SetAngularDamping(_InDamping); }
    float GetLinearDamping() const { return BodyInstance.GetLinearDamping(); }
    float GetAngularDamping() const { return BodyInstance.GetAngularDamping(); }
	void AddForce(FVector _Force, FName _BoneName = NAME_None, bool _bAccelChange = false) { BodyInstance.AddForce(_Force, true, _bAccelChange); }
	TMulticastDelegate<void(UPrimitiveComponent*, AActor*, UPrimitiveComponent*, int32, bool, const FHitResult&)> OnComponentBeginOverlap;
	TMulticastDelegate<void(UPrimitiveComponent*, AActor*, UPrimitiveComponent*, int32)> OnComponentEndOverlap;
	TMulticastDelegate<void(UPrimitiveComponent*, AActor*, UPrimitiveComponent*, FVector, const FHitResult&)> OnComponentHit;
	virtual ~UPrimitiveComponent() override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() const { return nullptr; }
    virtual UMaterialInterface* GetMaterial(int32 _ElementIndex) const { return nullptr; }
    virtual void SetMaterial(int32 _ElementIndex, UMaterialInterface* _Material) {}
    virtual int32 GetNumMaterials() const { return 0; }
    UMaterialInstanceDynamic* CreateDynamicMaterialInstance(int32 _ElementIndex, UMaterialInterface* _SourceMaterial = nullptr, FName _OptionalName = FName());

	virtual void OnRegister() override;
	virtual void OnUnregister() override;

protected:
	virtual void OnUpdateTransform() override;
private:
	bool __GenerateOverlapEvents = true;
};
