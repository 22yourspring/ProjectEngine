template<typename T, typename... Args>
T* UWorld::SpawnActor(Args&&... _Args)
{
	static_assert(std::is_base_of<AActor, T>::value, "T must derive from AActor.");
	std::lock_guard<std::recursive_mutex> Lock(__WorldMutex);

	T* Actor = __PersistentLevel->SpawnActor<T>(std::forward<Args>(_Args)...);

	RegisterActorTickFunctions(Actor);

	return Actor;
}

template<typename T>
T* UWorld::GetAuthGameMode() const
{
	static_assert(std::is_base_of<AGameModeBase, T>::value,
		"T must derive from AGameModeBase.");

	return dynamic_cast<T*>(__GameMode.get());
}
