#include "pch.h"
#include "SceneComponent.h"
#include <algorithm>

namespace
{
	FVector AddLocation(const FVector& _Left, const FVector& _Right)
	{
		return
		{
			_Left.X + _Right.X,
			_Left.Y + _Right.Y,
			_Left.Z + _Right.Z
		};
	}

	FVector SubtractLocation(const FVector& _Left, const FVector& _Right)
	{
		return
		{
			_Left.X - _Right.X,
			_Left.Y - _Right.Y,
			_Left.Z - _Right.Z
		};
	}
}

USceneComponent::~USceneComponent()
{
	DetachFromComponent();

	std::vector<USceneComponent*> Children = __AttachChildren;
	for (USceneComponent* Child : Children)
	{
		if (nullptr != Child)
			Child->DetachFromComponent();
	}
}

void USceneComponent::SetRelativeLocation(const FVector& _Location)
{
	__RelativeLocation = _Location;
	UpdateComponentToWorld();
	PropagateTransformToChildren();
	OnUpdateTransform();
}

void USceneComponent::SetWorldLocation(const FVector& _Location)
{
	__WorldLocation = _Location;

	if (nullptr != __AttachParent)
	{
		__RelativeLocation = __AttachParent->GetComponentTransform().InverseTransformPosition(__WorldLocation);
	}
	else
	{
		__RelativeLocation = __WorldLocation;
	}

	PropagateTransformToChildren();
	OnUpdateTransform();
}

bool USceneComponent::SetupAttachment(USceneComponent* _Parent)
{
	if (_Parent == this ||
		(nullptr != _Parent && _Parent->IsAttachedTo(this)))
		return false;

	if (__AttachParent == _Parent)
		return true;

	DetachFromComponent();
	__AttachParent = _Parent;

	if (nullptr != __AttachParent)
		__AttachParent->__AttachChildren.push_back(this);

	UpdateComponentToWorld();
	PropagateTransformToChildren();
	OnUpdateTransform();
	return true;
}

void USceneComponent::DetachFromComponent()
{
	if (nullptr == __AttachParent)
		return;

	const FVector PreviousWorldLocation = __WorldLocation;
	std::vector<USceneComponent*>& Siblings = __AttachParent->__AttachChildren;
	Siblings.erase(
		std::remove(Siblings.begin(), Siblings.end(), this),
		Siblings.end());

	__AttachParent = nullptr;
	__RelativeLocation = PreviousWorldLocation;
	__RelativeRotation = __WorldRotation;
	__RelativeScale = __WorldScale;
	__WorldLocation = PreviousWorldLocation;
	PropagateTransformToChildren();
	OnUpdateTransform();
}

bool USceneComponent::IsAttachedTo(const USceneComponent* _Component) const
{
	for (const USceneComponent* Parent = __AttachParent;
		nullptr != Parent;
		Parent = Parent->__AttachParent)
	{
		if (Parent == _Component)
			return true;
	}

	return false;
}

void USceneComponent::UpdateComponentToWorld()
{
	__WorldLocation = __AttachParent ? __AttachParent->GetComponentTransform().TransformPosition(__RelativeLocation) : __RelativeLocation;
	__WorldRotation = __AttachParent ? (__AttachParent->__WorldRotation * __RelativeRotation).GetNormalized() : __RelativeRotation;
	__WorldScale = __AttachParent ? __AttachParent->__WorldScale * __RelativeScale : __RelativeScale;
}

void USceneComponent::SetWorldRotation(const FQuat& _NewRotation)
{
    if (_NewRotation.ContainsNaN() || _NewRotation.SizeSquared() < 1.e-12) return;
    __RelativeRotation = __AttachParent ? __AttachParent->__WorldRotation.Inverse() * _NewRotation.GetNormalized() : _NewRotation.GetNormalized();
    UpdateComponentToWorld(); PropagateTransformToChildren(); OnUpdateTransform();
}
void USceneComponent::SetRelativeRotation(const FQuat& _NewRotation)
{
    if (_NewRotation.ContainsNaN() || _NewRotation.SizeSquared() < 1.e-12) return;
    __RelativeRotation = _NewRotation.IsNormalized() ? _NewRotation : _NewRotation.GetNormalized();
    UpdateComponentToWorld(); PropagateTransformToChildren(); OnUpdateTransform();
}
void USceneComponent::SetRelativeScale3D(FVector _NewScale3D)
{
    if (_NewScale3D.ContainsNaN()) return;
    __RelativeScale = _NewScale3D;
    UpdateComponentToWorld(); PropagateTransformToChildren(); OnUpdateTransform();
}

void USceneComponent::PropagateTransformToChildren()
{
	for (USceneComponent* Child : __AttachChildren)
	{
		if (nullptr == Child)
			continue;

		Child->UpdateComponentToWorld();
		Child->PropagateTransformToChildren();
		Child->OnUpdateTransform();
	}
}
