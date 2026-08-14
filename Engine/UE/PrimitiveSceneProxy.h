#pragma once

#include "Math/Vector.h"

class FDynamicRHI;

class FPrimitiveSceneProxy
{
public:
	virtual ~FPrimitiveSceneProxy() = default;

	virtual void SetWorldLocation(const FVector& _Location) { __WorldLocation = _Location; }
	virtual void Draw(FDynamicRHI& _DynamicRHI) const = 0;

protected:
	FVector __WorldLocation = {};
};
