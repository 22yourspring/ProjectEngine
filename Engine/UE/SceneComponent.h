#pragma once

#include "ActorComponent.h"
#include "Math/Vector.h"

UCLASS(MinimalAPI)
class USceneComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	virtual ~USceneComponent() override;

	void SetRelativeLocation(const FVector& _Location);
	const FVector& GetRelativeLocation() const { return __RelativeLocation; }

	void SetWorldLocation(const FVector& _Location);
	const FVector& GetWorldLocation() const { return __WorldLocation; }

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

	FVector __RelativeLocation = {};
	FVector __WorldLocation = {};
	USceneComponent* __AttachParent = nullptr;
	std::vector<USceneComponent*> __AttachChildren;
};
