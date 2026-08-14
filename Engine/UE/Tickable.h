#pragma once

#include "pch.h"

class AActor;
class UActorComponent;

enum class ETickingGroup
{
	TG_PrePhysics,
	TG_StartPhysics,
	TG_DuringPhysics,
	TG_EndPhysics,
	TG_PostPhysics,
	TG_PostUpdateWork
};

struct FTickFunction
{
	bool bCanEverTick = true;
	ETickingGroup TickGroup = ETickingGroup::TG_PrePhysics;
};

struct FActorTickFunction : public FTickFunction
{
	void ExecuteTick(AActor* _Target, float _DeltaTime);
};

struct FComponentTickFunction : public FTickFunction
{
	void ExecuteTick(UActorComponent* _Target, float _DeltaTime);
};

class FTickableGameObject
{
public:
	virtual ~FTickableGameObject() = default;

	virtual void Tick(float _DeltaTime) = 0;
	virtual bool IsTickable() const { return true; }
};
