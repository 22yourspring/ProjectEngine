#pragma once

template<typename T>
void TNativeClassRegistration<T>::Register(const FString& _Name)
{
    if constexpr (std::is_default_constructible_v<T> && !std::is_abstract_v<T>)
    {
        if constexpr (std::is_base_of_v<AActor, T> && !std::is_base_of_v<AGameModeBase, T>)
            FWorldPersistence::RegisterActor(_Name, typeid(T), [](UWorld* _World) { return _World->SpawnActor<T>(); });
        else if constexpr (std::is_base_of_v<UActorComponent, T>)
            FWorldPersistence::RegisterComponent(_Name, typeid(T), [](AActor* _Actor) { return _Actor->CreateInstanceComponent<T>(); });
    }
}

template<typename T>
void TNativeClassRegistration<T>::Unregister(const FString& _Name)
{
    if constexpr ((std::is_base_of_v<AActor, T> && !std::is_base_of_v<AGameModeBase, T>) || std::is_base_of_v<UActorComponent, T>)
        FWorldPersistence::UnregisterClass(_Name);
}
