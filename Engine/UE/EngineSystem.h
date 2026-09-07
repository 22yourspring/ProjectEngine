#pragma once

#include "pch.h"
#include "Tickable.h"

UCLASS(Abstract)
class ENGINE_API IEngineSystem : public FTickableGameObject
{
	GENERATED_BODY()

public:
	FEngineSystemTickFunction PrimaryEngineSystemTick;

	virtual ~IEngineSystem() = default;
	virtual HRESULT Initialize() = 0;
	virtual void Deinitialize() = 0;
	virtual bool UsesTickGroup() const { return false; }
};
