#pragma once

#include "UE/InputTypes.h"

#include <vector>

PROJECT_API bool InitializeProject();
PROJECT_API bool InitializeProjectWithGameMode(bool _UseEngineGameMode);
PROJECT_API bool SetProjectPaused(bool _bPaused);
PROJECT_API bool StopProject();
PROJECT_API void LoadProjectInputMappings();
PROJECT_API bool SetProjectActionMapping(const char* _MappingName, EKey _Key);
PROJECT_API bool SetProjectAxisMapping(const char* _MappingName, EKey _Key, float _Scale);
PROJECT_API bool RemoveProjectActionMapping(const FInputActionKeyMapping& _Mapping);
PROJECT_API bool RemoveProjectAxisMapping(const FInputAxisKeyMapping& _Mapping);
PROJECT_API std::vector<FInputActionKeyMapping> GetProjectActionMappings();
PROJECT_API std::vector<FInputAxisKeyMapping> GetProjectAxisMappings();
