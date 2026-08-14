#pragma once

#include "UE/InputTypes.h"

#include <vector>

bool InitializeProject();
bool SetProjectActionMapping(const char* _MappingName, EKey _Key);
bool SetProjectAxisMapping(const char* _MappingName, EKey _Key, float _Scale);
bool RemoveProjectActionMapping(const FInputActionKeyMapping& _Mapping);
bool RemoveProjectAxisMapping(const FInputAxisKeyMapping& _Mapping);
std::vector<FInputActionKeyMapping> GetProjectActionMappings();
std::vector<FInputAxisKeyMapping> GetProjectAxisMappings();
