#pragma once

#include "UE/UnrealString.h"
#include <filesystem>
#include <string>
#include <vector>

struct FNativeClassSource
{
    FString __Name;
    FString __Module;
    std::filesystem::path __Header;
    std::filesystem::path __Folder;
    int __Line = 1;
    bool __Engine = false;
};

std::vector<FNativeClassSource> ReadNativeClassSources(const std::filesystem::path& _Project, const FString& _Module, bool _Engine);
bool OpenNativeClassSource(const std::filesystem::path& _Solution, const FNativeClassSource& _Class, FString& _Error, bool _LaunchIfMissing = true);
