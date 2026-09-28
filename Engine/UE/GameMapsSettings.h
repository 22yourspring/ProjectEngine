#pragma once
#include "Object.h"
#include <filesystem>

class ENGINE_API UGameMapsSettings : public UObject
{
public:
    FString __EditorStartupMap;
    FString __GameDefaultMap = TEXT("/Game/Levels/Stage1");
    bool __UseEngineGameMode = false;
    void Load(const std::filesystem::path& _Project);
    bool Save(const std::filesystem::path& _Project) const;
    static std::filesystem::path ResolveMap(const std::filesystem::path& _Content, const FString& _Map);
    static FString MapName(const std::filesystem::path& _Content, const std::filesystem::path& _File);
};
