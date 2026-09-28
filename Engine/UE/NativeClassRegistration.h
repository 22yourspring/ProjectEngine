#pragma once
#include "WorldPersistence.h"
#include "World.h"
#include "GameModeBase.h"
#include <type_traits>

template<typename T>
class TNativeClassRegistration
{
public:
    static void Register(const FString& _Name);
    static void Unregister(const FString& _Name);
};

#include "NativeClassRegistration.inl"
