#pragma once

#include "define.h"
#include "InputTypes.h"
#include "GameModeBase.h"
#include <vector>

class IModuleInterface
{
public:
    virtual ~IModuleInterface() = default;
    virtual std::unique_ptr<AGameModeBase> CreateGameMode(bool _UseEngineGameMode) = 0;
    virtual bool InitializeProject() = 0;
    virtual bool InitializeProjectWithGameMode(bool _UseEngineGameMode) = 0;
    virtual bool SetProjectPaused(bool _bPaused) = 0;
    virtual bool StopProject() = 0;
    virtual void LoadProjectInputMappings() = 0;
    virtual bool SetProjectActionMapping(const FString& _MappingName, EKey _Key) = 0;
    virtual bool SetProjectAxisMapping(const FString& _MappingName, EKey _Key, float _Scale) = 0;
    virtual bool RemoveProjectActionMapping(const FInputActionKeyMapping& _Mapping) = 0;
    virtual bool RemoveProjectAxisMapping(const FInputAxisKeyMapping& _Mapping) = 0;
    virtual std::vector<FInputActionKeyMapping> GetProjectActionMappings() = 0;
    virtual std::vector<FInputAxisKeyMapping> GetProjectAxisMappings() = 0;
};

inline constexpr unsigned int GameModuleApiVersion = 3;
