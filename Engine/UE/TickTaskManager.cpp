#include "pch.h"
#include "TickTaskManager.h"
#include "Actor.h"
#include "ActorComponent.h"
#include "EngineSystem.h"
#include <algorithm>

void FEngineSystemTickFunction::ExecuteTick(IEngineSystem* _Target, float _DeltaTime)
{
	if (nullptr != _Target && bCanEverTick && _Target->IsTickable())
		_Target->Tick(_DeltaTime);
}

void FTickTaskManager::AddTickFunction(FActorTickFunction* F, AActor* T)
{
	if (!F || !T)
		return;

	if (__bRunningTick) 
		__PendingAddActors.push_back({ F, T });
	
	else 
		__ActorTickFunctions.push_back({ F, T });
}

void FTickTaskManager::AddTickFunction(FComponentTickFunction* F, UActorComponent* T)
{
	if (!F || !T) 
		return;

	if (__bRunningTick) 
		__PendingAddComponents.push_back({ F, T });

	else 
		__ComponentTickFunctions.push_back({ F, T });
}

void FTickTaskManager::AddTickFunction(FEngineSystemTickFunction* F, IEngineSystem* T)
{
	if (!F || !T)
		return;

	if (__bRunningTick)
		__PendingAddEngineSystems.push_back({ F, T });
	else
		__EngineSystemTickFunctions.push_back({ F, T });
}

void FTickTaskManager::RemoveTickFunction(FActorTickFunction* F)
{
	if (!F)
		return;

	if (__bRunningTick)
	{
		__PendingRemoveActors.push_back(F); 
		return;
	}

	auto Iter = std::remove_if 
	(
		__ActorTickFunctions.begin(),
		__ActorTickFunctions.end(), 
		[F](const FActorEntry& E) 
		{ 
			return E.TickFunction == F;
		}
	);

	__ActorTickFunctions.erase(Iter, __ActorTickFunctions.end());
}

void FTickTaskManager::RemoveTickFunction(FComponentTickFunction* F)
{
	if (!F) 
		return;

	if (__bRunningTick)
	{
		__PendingRemoveComponents.push_back(F); 
		return; 
	}

	auto Iter = std::remove_if
	(
		__ComponentTickFunctions.begin(),
		__ComponentTickFunctions.end(),
		[F](const FComponentEntry& E) 
		{
			return E.TickFunction == F; 
		}
	);

	__ComponentTickFunctions.erase(Iter, __ComponentTickFunctions.end());
}

void FTickTaskManager::RemoveTickFunction(FEngineSystemTickFunction* F)
{
	if (!F)
		return;

	if (__bRunningTick)
	{
		__PendingRemoveEngineSystems.push_back(F);
		return;
	}

	auto Iter = std::remove_if
	(
		__EngineSystemTickFunctions.begin(),
		__EngineSystemTickFunctions.end(),
		[F](const FEngineSystemEntry& E)
		{
			return E.TickFunction == F;
		}
	);

	__EngineSystemTickFunctions.erase(Iter, __EngineSystemTickFunctions.end());
}

void FTickTaskManager::RunTickGroup(ETickingGroup _TickGroup, float _DeltaTime)
{
	__bRunningTick = true;

	for (const FActorEntry& Entry : __ActorTickFunctions)
	{
		if (Entry.TickFunction->TickGroup == _TickGroup)
			Entry.TickFunction->ExecuteTick(Entry.Target, _DeltaTime);
	}

	for (const FComponentEntry& Entry : __ComponentTickFunctions)
	{
		if (Entry.TickFunction->TickGroup == _TickGroup)
			Entry.TickFunction->ExecuteTick(Entry.Target, _DeltaTime);
	}

	for (const FEngineSystemEntry& Entry : __EngineSystemTickFunctions)
	{
		if (Entry.TickFunction->TickGroup == _TickGroup)
			Entry.TickFunction->ExecuteTick(Entry.Target, _DeltaTime);
	}

	__bRunningTick = false;
	
	FlushPendingTickFunctions();
}
void FTickTaskManager::FlushPendingTickFunctions()
{
	__ActorTickFunctions.insert(__ActorTickFunctions.end(), __PendingAddActors.begin(), __PendingAddActors.end());
	__ComponentTickFunctions.insert(__ComponentTickFunctions.end(), __PendingAddComponents.begin(), __PendingAddComponents.end());
	__EngineSystemTickFunctions.insert(__EngineSystemTickFunctions.end(), __PendingAddEngineSystems.begin(), __PendingAddEngineSystems.end());
	
	__PendingAddActors.clear();
	__PendingAddComponents.clear();
	__PendingAddEngineSystems.clear();
	
	for (auto* F : __PendingRemoveActors) 
		RemoveTickFunction(F);

	for (auto* F : __PendingRemoveComponents) 
		RemoveTickFunction(F);

	for (auto* F : __PendingRemoveEngineSystems)
		RemoveTickFunction(F);

	__PendingRemoveActors.clear(); 
	__PendingRemoveComponents.clear();
	__PendingRemoveEngineSystems.clear();
}
