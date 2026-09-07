#pragma once

#include "Tickable.h"

class FTickTaskManager
{
public:
	void AddTickFunction(FActorTickFunction* _TickFunction, AActor* _Target);
	void AddTickFunction(FComponentTickFunction* _TickFunction, UActorComponent* _Target);
	void AddTickFunction(FEngineSystemTickFunction* _TickFunction, IEngineSystem* _Target);
	void RemoveTickFunction(FActorTickFunction* _TickFunction);
	void RemoveTickFunction(FComponentTickFunction* _TickFunction);
	void RemoveTickFunction(FEngineSystemTickFunction* _TickFunction);
	void RunTickGroup(ETickingGroup _TickGroup, float _DeltaTime);

private:
	struct FActorEntry 
	{ 
		FActorTickFunction* TickFunction;
		AActor* Target;
	};
	
	struct FComponentEntry 
	{ 
		FComponentTickFunction* TickFunction; 
		UActorComponent* Target; 
	};

	struct FEngineSystemEntry
	{
		FEngineSystemTickFunction* TickFunction;
		IEngineSystem* Target;
	};
	
	void FlushPendingTickFunctions();

	std::vector<FActorEntry>				__ActorTickFunctions;
	std::vector<FComponentEntry>			__ComponentTickFunctions;
	std::vector<FEngineSystemEntry>			__EngineSystemTickFunctions;
	std::vector<FActorEntry>				__PendingAddActors;
	std::vector<FComponentEntry>			__PendingAddComponents;
	std::vector<FEngineSystemEntry>			__PendingAddEngineSystems;
	std::vector<FActorTickFunction*>		__PendingRemoveActors;
	std::vector<FComponentTickFunction*>	__PendingRemoveComponents;
	std::vector<FEngineSystemTickFunction*>	__PendingRemoveEngineSystems;

	bool __bRunningTick = false;
};
