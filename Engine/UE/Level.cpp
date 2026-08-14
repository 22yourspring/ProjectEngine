#include "pch.h"
#include "Level.h"
#include <algorithm>

ULevel::ULevel(UWorld* _OwningWorld)
	: __OwningWorld(_OwningWorld)
{
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
