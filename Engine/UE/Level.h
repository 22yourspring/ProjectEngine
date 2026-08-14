#pragma once

#include "Object.h"
#include "Actor.h"
#include <type_traits>

class UWorld;
class ULevel : public UObject
{
public:
	explicit ULevel(UWorld* _OwningWorld);

	template<typename T, typename... Args>
	T* SpawnActor(Args&&... _Args);

	bool DestroyActor(AActor* _Actor);

	UWorld* GetWorld() const { return __OwningWorld; }

private:
	UWorld*	__OwningWorld = nullptr;
	std::vector<std::unique_ptr<AActor>>	__Actors;
};

#include "Level.inl"
