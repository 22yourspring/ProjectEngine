#include "pch.h"
#include "GameMapsSettings.h"
#include <fstream>

namespace
{
    const wchar_t* Section = L"/Script/EngineSettings.GameMapsSettings";
    FString Read(const std::filesystem::path& _File, const wchar_t* _Key, const wchar_t* _Default)
    {
        wchar_t Buffer[32768] = {};
        GetPrivateProfileStringW(Section, _Key, _Default, Buffer, 32768, _File.c_str());
        return Buffer;
    }
}
void UGameMapsSettings::Load(const std::filesystem::path& _Project)
{
    const auto File = _Project / L"Config/DefaultEngine.ini";
    __EditorStartupMap = Read(File, L"EditorStartupMap", L"");
    __GameDefaultMap = Read(File, L"GameDefaultMap", L"/Game/Levels/Stage1");
    std::string Legacy;
    std::ifstream(_Project / L"Saved/Config/EditorDefaultPawn.cfg") >> Legacy;
    const auto Mode = Read(File, L"GlobalDefaultGameMode", Legacy == "EngineGameMode" ? L"/Script/UE.GameModeBase" : L"/Script/Project.ProjectGameMode");
    __UseEngineGameMode = Mode == TEXT("/Script/UE.GameModeBase");
}
bool UGameMapsSettings::Save(const std::filesystem::path& _Project) const
{
    const auto File = _Project / L"Config/DefaultEngine.ini";
    std::error_code Error; std::filesystem::create_directories(File.parent_path(), Error);
    if (Error) return false;
    return WritePrivateProfileStringW(Section, L"EditorStartupMap", *__EditorStartupMap, File.c_str()) &&
        WritePrivateProfileStringW(Section, L"GameDefaultMap", *__GameDefaultMap, File.c_str()) &&
        WritePrivateProfileStringW(Section, L"GlobalDefaultGameMode", __UseEngineGameMode ? L"/Script/UE.GameModeBase" : L"/Script/Project.ProjectGameMode", File.c_str());
}
std::filesystem::path UGameMapsSettings::ResolveMap(const std::filesystem::path& _Content, const FString& _Map)
{
    const FString& Text = _Map;
    if (!Text.StartsWith(TEXT("/Game/"), ESearchCase::CaseSensitive)) return {};
    auto Relative = std::filesystem::path(Text.Mid(6).ToWide()).lexically_normal();
    if (Relative.empty() || Relative.is_absolute() || *Relative.begin() == L"..") return {};
    Relative.replace_extension(L".umap"); return _Content / Relative;
}
FString UGameMapsSettings::MapName(const std::filesystem::path& _Content, const std::filesystem::path& _File)
{
    auto Relative = _File.lexically_normal().lexically_relative(_Content.lexically_normal());
    if (Relative.empty() || Relative.is_absolute() || *Relative.begin() == L"..") return {};
    Relative.replace_extension(); return FString(L"/Game/" + Relative.generic_wstring());
}
