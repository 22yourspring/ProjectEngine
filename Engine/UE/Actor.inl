#pragma once

template <typename ComponentType, typename... Args>
ComponentType* AActor::CreateDefaultSubobject(Args&&... args)
{
	ComponentType* NewComp = new ComponentType(std::forward<Args>(args)...);
	NewComp->SetOwner(this);
	__Components.emplace_back(NewComp);
	RegisterComponentTickFunction(NewComp);
	if (UWorld* World = GetWorld())
		NewComp->RegisterComponentWithWorld(World);

	return NewComp;
}
