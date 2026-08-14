#pragma once

#include "Object.h"
#include "DynamicRHI.h"

class UStaticMesh : public UObject
{
public:
	void SetSize(int32 _Width, int32 _Height) { __Width = _Width; __Height = _Height; }
	void SetColor(const FColor& _Color) { __Color = _Color; }

	int32 GetWidth() const { return __Width; }
	int32 GetHeight() const { return __Height; }
	const FColor& GetColor() const { return __Color; }

private:
	int32	__Width = 100;
	int32	__Height = 100;
	FColor	__Color = { 220, 60, 60, 255 };
};
