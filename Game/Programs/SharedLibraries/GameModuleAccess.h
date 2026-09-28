#pragma once

#include "UE/ModuleManager.h"

inline bool InitializeProject()
{
    if (IModuleInterface* Module = FModuleManager::GetGameModule())
        return Module->InitializeProject();
    return {};
}

inline bool InitializeProjectWithGameMode(bool _UseEngineGameMode)
{
    if (IModuleInterface* Module = FModuleManager::GetGameModule())
        return Module->InitializeProjectWithGameMode(_UseEngineGameMode);
    return {};
}

inline bool SetProjectPaused(bool _bPaused)
{
    if (IModuleInterface* Module = FModuleManager::GetGameModule())
        return Module->SetProjectPaused(_bPaused);
    return {};
}

inline bool StopProject()
{
    if (IModuleInterface* Module = FModuleManager::GetGameModule())
        return Module->StopProject();
    return {};
}

inline void LoadProjectInputMappings()
{
    if (IModuleInterface* Module = FModuleManager::GetGameModule())
        return Module->LoadProjectInputMappings();
}

inline bool SetProjectActionMapping(const FString& _MappingName, EKey _Key)
{
    if (IModuleInterface* Module = FModuleManager::GetGameModule())
        return Module->SetProjectActionMapping(_MappingName, _Key);
    return {};
}

inline bool SetProjectAxisMapping(const FString& _MappingName, EKey _Key, float _Scale)
{
    if (IModuleInterface* Module = FModuleManager::GetGameModule())
        return Module->SetProjectAxisMapping(_MappingName, _Key, _Scale);
    return {};
}

inline bool RemoveProjectActionMapping(const FInputActionKeyMapping& _Mapping)
{
    if (IModuleInterface* Module = FModuleManager::GetGameModule())
        return Module->RemoveProjectActionMapping(_Mapping);
    return {};
}

inline bool RemoveProjectAxisMapping(const FInputAxisKeyMapping& _Mapping)
{
    if (IModuleInterface* Module = FModuleManager::GetGameModule())
        return Module->RemoveProjectAxisMapping(_Mapping);
    return {};
}

inline std::vector<FInputActionKeyMapping> GetProjectActionMappings()
{
    if (IModuleInterface* Module = FModuleManager::GetGameModule())
        return Module->GetProjectActionMappings();
    return {};
}

inline std::vector<FInputAxisKeyMapping> GetProjectAxisMappings()
{
    if (IModuleInterface* Module = FModuleManager::GetGameModule())
        return Module->GetProjectAxisMappings();
    return {};
}

