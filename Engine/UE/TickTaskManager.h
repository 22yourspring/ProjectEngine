#pragma once

#include "Tickable.h"

class FTickTaskManager
{
public:
	void AddTickFunction(FActorTickFunction* _TickFunction, AActor* _Target);
	void AddTickFunction(FComponentTickFunction* _TickFunction, UActorComponent* _Target);
	void RemoveTickFunction(FActorTickFunction* _TickFunction);
	void RemoveTickFunction(FComponentTickFunction* _TickFunction);
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
	
	void FlushPendingTickFunctions();

	std::vector<FActorEntry>				__ActorTickFunctions;
	std::vector<FComponentEntry>			__ComponentTickFunctions;
	std::vector<FActorEntry>				__PendingAddActors;
	std::vector<FComponentEntry>			__PendingAddComponents;
	std::vector<FActorTickFunction*>		__PendingRemoveActors;
	std::vector<FComponentTickFunction*>	__PendingRemoveComponents;

	bool __bRunningTick = false;
};
