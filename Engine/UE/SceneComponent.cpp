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
		__RelativeLocation = SubtractLocation(
			__WorldLocation,
			__AttachParent->GetWorldLocation());
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
	__WorldLocation = nullptr != __AttachParent
		? AddLocation(__AttachParent->GetWorldLocation(), __RelativeLocation)
		: __RelativeLocation;
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
