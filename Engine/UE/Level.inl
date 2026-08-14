template<typename T, typename... Args>
T* ULevel::SpawnActor(Args&&... _Args)
{
	static_assert(std::is_base_of<AActor, T>::value, "T must derive from AActor.");

	auto NewActor = std::make_unique<T>(std::forward<Args>(_Args)...);
	T* Actor = NewActor.get();

	Actor->SetLevel(this);
	__Actors.emplace_back(std::move(NewActor));

	return Actor;
}
