#pragma once

#include "Math/Vector.h"
#include "Math/Transform.h"

class FDynamicRHI;

class FPrimitiveSceneProxy
{
public:
	virtual ~FPrimitiveSceneProxy() = default;

	virtual void SetWorldLocation(const FVector& _Location) { __WorldLocation = _Location; }
    virtual void SetTransform(const FTransform& _Transform) { SetWorldLocation(_Transform.GetLocation()); }
	virtual void Draw(FDynamicRHI& _DynamicRHI) const = 0;

protected:
	FVector __WorldLocation = FVector::ZeroVector;
};
