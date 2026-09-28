#pragma once

#include "ActorComponent.h"
#include "Math/Vector.h"
#include "Math/Transform.h"

UCLASS(MinimalAPI)
class ENGINE_API USceneComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	virtual ~USceneComponent() override;

	void SetRelativeLocation(const FVector& _Location);
	const FVector& GetRelativeLocation() const { return __RelativeLocation; }

	void SetWorldLocation(const FVector& _Location);
	const FVector& GetWorldLocation() const { return __WorldLocation; }
	FVector GetComponentLocation() const { return __WorldLocation; }
	FQuat GetComponentQuat() const { return __WorldRotation; }
	FVector GetComponentScale() const { return __WorldScale; }
	FQuat GetRelativeRotationQuaternion() const { return __RelativeRotation; }
	FVector GetRelativeScale3D() const { return __RelativeScale; }
	FTransform GetComponentTransform() const { return FTransform(__WorldRotation, __WorldLocation, __WorldScale); }
	void SetWorldRotation(const FQuat& _NewRotation);
	void SetRelativeRotation(const FQuat& _NewRotation);
	void SetRelativeScale3D(FVector _NewScale3D);

	bool SetupAttachment(USceneComponent* _Parent);
	void DetachFromComponent();
	USceneComponent* GetAttachParent() const { return __AttachParent; }
	const std::vector<USceneComponent*>& GetAttachChildren() const { return __AttachChildren; }

protected:
	virtual void OnUpdateTransform() {}

private:
	bool IsAttachedTo(const USceneComponent* _Component) const;
	void UpdateComponentToWorld();
	void PropagateTransformToChildren();

	FVector __RelativeLocation = FVector::ZeroVector;
	FVector __WorldLocation = FVector::ZeroVector;
	FQuat __RelativeRotation = FQuat::Identity;
	FQuat __WorldRotation = FQuat::Identity;
	FVector __RelativeScale = FVector(1, 1, 1);
	FVector __WorldScale = FVector(1, 1, 1);
	USceneComponent* __AttachParent = nullptr;
	std::vector<USceneComponent*> __AttachChildren;
};
