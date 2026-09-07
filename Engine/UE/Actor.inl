#pragma once

template <typename ComponentType, typename... Args>
ComponentType* AActor::CreateDefaultSubobject(Args&&... _Args)
{
	return CreateOwnedComponent<ComponentType>(
		false,
		std::forward<Args>(_Args)...);
}

template <typename ComponentType, typename... Args>
ComponentType* AActor::CreateInstanceComponent(Args&&... _Args)
{
	return CreateOwnedComponent<ComponentType>(
		true,
		std::forward<Args>(_Args)...);
}

template <typename ComponentType, typename... Args>
ComponentType* AActor::CreateOwnedComponent(
	bool _bInstanceComponent,
	Args&&... _Args)
{
	static_assert(std::is_base_of_v<UActorComponent, ComponentType>);

	auto Component = std::make_unique<ComponentType>(
		std::forward<Args>(_Args)...);
	ComponentType* NewComponent = Component.get();

	NewComponent->SetOwner(this);
	__ComponentStorage.push_back(std::move(Component));
	AddOwnedComponent(NewComponent);

	if (_bInstanceComponent)
		AddInstanceComponent(NewComponent);

	RegisterComponentTickFunction(NewComponent);
	if (UWorld* World = GetWorld())
		NewComponent->RegisterComponentWithWorld(World);

	return NewComponent;
}
