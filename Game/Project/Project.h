#pragma once

#include "UE/InputTypes.h"

#include <vector>

bool InitializeProject();
bool InitializeProjectWithGameMode(bool _UseEngineGameMode);
bool SetProjectPaused(bool _bPaused);
bool StopProject();
void LoadProjectInputMappings();
bool SetProjectActionMapping(const FString& _MappingName, EKey _Key);
bool SetProjectAxisMapping(const FString& _MappingName, EKey _Key, float _Scale);
bool RemoveProjectActionMapping(const FInputActionKeyMapping& _Mapping);
bool RemoveProjectAxisMapping(const FInputAxisKeyMapping& _Mapping);
std::vector<FInputActionKeyMapping> GetProjectActionMappings();
std::vector<FInputAxisKeyMapping> GetProjectAxisMappings();
