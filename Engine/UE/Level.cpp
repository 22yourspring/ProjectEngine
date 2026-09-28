#include "pch.h"
#include "Level.h"
#include <algorithm>

FString ULevel::GetMapName() const
{
    return __LevelName.ToString();
}

ULevel::ULevel(UWorld* _OwningWorld, const FName& _LevelName)
	: __OwningWorld(_OwningWorld), __LevelName(_LevelName)
{
}

void ULevel::Tick(float _DeltaTime)
{
	if (!__bVisible)
		return;

	for (const std::unique_ptr<AActor>& Actor : __Actors)
	{
		if (Actor && Actor->IsTickable())
			Actor->TickActor(_DeltaTime);
	}
}

void ULevel::RunTickGroup(ETickingGroup _TickGroup, float _DeltaTime)
{
	UNREFERENCED_PARAMETER(_TickGroup);

	if (!__bVisible)
		return;

	UNREFERENCED_PARAMETER(_DeltaTime);
}

void ULevel::Render()
{
	if (!__bVisible)
		return;

}

bool ULevel::DestroyActor(AActor* _Actor)
{
	if (nullptr == _Actor)
		return false;

	auto Iter = std::find_if
	(
		__Actors.begin(),
		__Actors.end(),
		[_Actor](const std::unique_ptr<AActor>& Actor)
		{
			return Actor.get() == _Actor;
		}
	);

	if (__Actors.end() == Iter)
		return false;

	(*Iter)->Destroyed();
	__Actors.erase(Iter);

	return true;
}
