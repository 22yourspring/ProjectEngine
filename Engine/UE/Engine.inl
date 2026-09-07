#pragma once

template<typename T>
T* UEngine::CreateEngineSystem()
{
    static_assert(std::is_base_of<IEngineSystem, T>::value, "T must derive from IEngineSystem.");

    if (__bGameLoopStarted.load(std::memory_order_acquire))
        return nullptr;

    return CreateEngineSystemInternal<T>(EEngineSystemInitializeReason::ExplicitPreload);
}

template<typename T>
T* UEngine::GetEngineSystem()
{
    static_assert(std::is_base_of<IEngineSystem, T>::value, "T must derive from IEngineSystem.");

    const std::type_index TypeIndex(typeid(T));
    T* EngineSystem = nullptr;

    {
        std::lock_guard<std::mutex> Lock(__EngineSystemMutex);

        auto Iter = __EngineSystems.find(TypeIndex);

        if (__EngineSystems.end() != Iter)
            EngineSystem = static_cast<T*>(Iter->second.Instance.get());
    }

    if (nullptr == EngineSystem)
        return CreateEngineSystemInternal<T>(EEngineSystemInitializeReason::RuntimeLazyAccess);

    if (false == InitializeEngineSystem<T>(
        EngineSystem, EEngineSystemInitializeReason::RuntimeLazyAccess))
        return nullptr;

    return EngineSystem;
}

template<typename T>
T* UEngine::CreateEngineSystemInternal(EEngineSystemInitializeReason _Reason)
{
    static_assert(std::is_base_of<IEngineSystem, T>::value, "T must derive from IEngineSystem.");

    const std::type_index TypeIndex(typeid(T));
    std::unique_ptr<T> NewEngineSystem = std::make_unique<T>();
    T* NewEngineSystemPointer = NewEngineSystem.get();
    T* EngineSystem = nullptr;

    {
        std::lock_guard<std::mutex> Lock(__EngineSystemMutex);
        FEngineSystemEntry& Entry = __EngineSystems[TypeIndex];

        if (nullptr == Entry.Instance)
        {
            Entry.Instance = std::move(NewEngineSystem);
            EngineSystem = NewEngineSystemPointer;
        }
        else
        {
            EngineSystem = static_cast<T*>(Entry.Instance.get());
        }
    }

    if (false == InitializeEngineSystem<T>(EngineSystem, _Reason))
        return nullptr;

    return EngineSystem;
}

template<typename T>
bool UEngine::InitializeEngineSystem(T* _EngineSystem, EEngineSystemInitializeReason _Reason)
{
    static_assert(std::is_base_of<IEngineSystem, T>::value, "T must derive from IEngineSystem.");

    return InitializeEngineSystemEntry(std::type_index(typeid(T)), _EngineSystem, typeid(T).name(), _Reason);
}
