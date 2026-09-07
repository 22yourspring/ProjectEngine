#include "pch.h"
#include "LevelStreaming.h"
#include "Level.h"

ULevelStreaming::ULevelStreaming(const FName& _PackageName)
	: __PackageName(_PackageName)
{
}

ULevelStreaming::ULevelStreaming(const TCHAR* _PackageName)
	: __PackageName(_PackageName)
{
}

ULevelStreaming::~ULevelStreaming() = default;

void ULevelStreaming::SetLoadedLevel(std::unique_ptr<ULevel> _Level)
{
	__LoadedLevel = std::move(_Level);
}

void ULevelStreaming::UnloadLevel()
{
	__LoadedLevel.reset();
}
