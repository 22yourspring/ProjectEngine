#pragma once

template<typename T>
T* UEngine::CreateSubsystem()
{
    static_assert(std::is_base_of<ISubsystem, T>::value, "T must derive from ISubsystem.");

    if (__bGameLoopStarted.load(std::memory_order_acquire))
        return nullptr;

    return CreateSubsystemInternal<T>(ESubsystemInitializeReason::ExplicitPreload);
}

template<typename T>
T* UEngine::GetSubsystem()
{
    static_assert(std::is_base_of<ISubsystem, T>::value, "T must derive from ISubsystem.");

    const std::type_index TypeIndex(typeid(T));
    T* Subsystem = nullptr;

    {
        std::lock_guard<std::mutex> Lock(__SubsystemMutex);

        auto Iter = __Subsystems.find(TypeIndex);

        if (__Subsystems.end() != Iter)
            Subsystem = static_cast<T*>(Iter->second.Instance.get());
    }

    if (nullptr == Subsystem)
        return CreateSubsystemInternal<T>(ESubsystemInitializeReason::RuntimeLazyAccess);

    if (false == InitializeSubsystem<T>(
        Subsystem, ESubsystemInitializeReason::RuntimeLazyAccess))
        return nullptr;

    return Subsystem;
}

template<typename T>
T* UEngine::CreateSubsystemInternal(ESubsystemInitializeReason _Reason)
{
    static_assert(std::is_base_of<ISubsystem, T>::value, "T must derive from ISubsystem.");

    const std::type_index TypeIndex(typeid(T));
    std::unique_ptr<T> NewSubsystem = std::make_unique<T>();
    T* NewSubsystemPointer = NewSubsystem.get();
    T* Subsystem = nullptr;

    {
        std::lock_guard<std::mutex> Lock(__SubsystemMutex);
        FSubsystemEntry& Entry = __Subsystems[TypeIndex];

        if (nullptr == Entry.Instance)
        {
            Entry.Instance = std::move(NewSubsystem);
            Subsystem = NewSubsystemPointer;
        }
        else
        {
            Subsystem = static_cast<T*>(Entry.Instance.get());
        }
    }

    if (false == InitializeSubsystem<T>(Subsystem, _Reason))
        return nullptr;

    return Subsystem;
}

template<typename T>
bool UEngine::InitializeSubsystem(T* _Subsystem, ESubsystemInitializeReason _Reason)
{
    static_assert(std::is_base_of<ISubsystem, T>::value, "T must derive from ISubsystem.");

    return InitializeSubsystemEntry(std::type_index(typeid(T)), _Subsystem, typeid(T).name(), _Reason);
}
