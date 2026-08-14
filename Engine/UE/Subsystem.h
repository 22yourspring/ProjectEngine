#pragma once

#include "pch.h"
#include "Tickable.h"

UCLASS(Abstract)
class ENGINE_API ISubsystem : public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual ~ISubsystem() = default;
	virtual HRESULT Initialize() = 0;
	virtual void Deinitialize() = 0;
};
