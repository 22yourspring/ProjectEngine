#include "pch.h"
#include "CollisionProfile.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <set>
#include <cctype>

namespace
{
    bool ValidName(FName _Name)
    {
        const auto Text = _Name.ToString().ToUtf8();
        return !Text.empty() && Text.size() <= 128 && std::none_of(Text.begin(), Text.end(), [](unsigned char _Char) { return std::isspace(_Char) != 0 || _Char < 32; });
    }
}

UCollisionProfile* UCollisionProfile::Get() { static UCollisionProfile Profile; return &Profile; }
void UCollisionProfile::ResetToDefaults()
{
    UCollisionProfile Defaults; __Profiles = std::move(Defaults.__Profiles); __Channels = std::move(Defaults.__Channels); ++__Revision;
}
UCollisionProfile::UCollisionProfile()
{
    const char* Names[] = {"WorldStatic", "WorldDynamic", "Pawn", "Visibility", "Camera", "PhysicsBody", "Vehicle", "Destructible"};
    for (int Index = 0; Index < 8; ++Index) __Channels.push_back({static_cast<ECollisionChannel>(Index), ECR_Block, Index == 3 || Index == 4, Index == 0, FName(Names[Index])});
    auto Add = [&](const char* _Name, ECollisionEnabled::Type _Enabled, ECollisionChannel _Object, ECollisionResponse _Default,
        std::initializer_list<std::pair<ECollisionChannel, ECollisionResponse>> _Overrides = {})
    {
        FCollisionResponseTemplate Entry;
        Entry.Name = FName(_Name); Entry.CollisionEnabled = _Enabled; Entry.ObjectType = _Object; Entry.bCanModify = false;
        for (int Index = 0; Index < 8; ++Index) Entry.ResponseToChannels.SetResponse(static_cast<ECollisionChannel>(Index), _Default);
        for (auto [Channel, Response] : _Overrides) Entry.ResponseToChannels.SetResponse(Channel, Response);
        __Profiles.push_back(Entry);
    };
    using namespace ECollisionEnabled;
    Add("NoCollision", NoCollision, ECC_WorldStatic, ECR_Block, {{ECC_Visibility,ECR_Ignore},{ECC_Camera,ECR_Ignore}});
    Add("BlockAll", QueryAndPhysics, ECC_WorldStatic, ECR_Block);
    Add("OverlapAll", QueryOnly, ECC_WorldStatic, ECR_Overlap);
    Add("BlockAllDynamic", QueryAndPhysics, ECC_WorldDynamic, ECR_Block);
    Add("OverlapAllDynamic", QueryOnly, ECC_WorldDynamic, ECR_Overlap);
    Add("IgnoreOnlyPawn", QueryOnly, ECC_WorldDynamic, ECR_Block, {{ECC_Pawn,ECR_Ignore},{ECC_Vehicle,ECR_Ignore}});
    Add("OverlapOnlyPawn", QueryOnly, ECC_WorldDynamic, ECR_Block, {{ECC_Pawn,ECR_Overlap},{ECC_Vehicle,ECR_Overlap},{ECC_Camera,ECR_Ignore}});
    Add("Pawn", QueryAndPhysics, ECC_Pawn, ECR_Block, {{ECC_Visibility,ECR_Ignore}});
    Add("Spectator", QueryOnly, ECC_Pawn, ECR_Ignore, {{ECC_WorldStatic,ECR_Block}});
    Add("CharacterMesh", QueryOnly, ECC_Pawn, ECR_Block, {{ECC_Pawn,ECR_Ignore},{ECC_Vehicle,ECR_Ignore},{ECC_Visibility,ECR_Ignore}});
    Add("PhysicsActor", QueryAndPhysics, ECC_PhysicsBody, ECR_Block);
    Add("Destructible", QueryAndPhysics, ECC_Destructible, ECR_Block);
    Add("InvisibleWall", QueryAndPhysics, ECC_WorldStatic, ECR_Block, {{ECC_Visibility,ECR_Ignore}});
    Add("InvisibleWallDynamic", QueryAndPhysics, ECC_WorldDynamic, ECR_Block, {{ECC_Visibility,ECR_Ignore}});
    Add("Trigger", QueryOnly, ECC_WorldDynamic, ECR_Overlap, {{ECC_Visibility,ECR_Ignore}});
    Add("Ragdoll", QueryAndPhysics, ECC_PhysicsBody, ECR_Block, {{ECC_Pawn,ECR_Ignore},{ECC_Visibility,ECR_Ignore}});
    Add("Vehicle", QueryAndPhysics, ECC_Vehicle, ECR_Block);
    Add("UI", QueryOnly, ECC_WorldDynamic, ECR_Overlap, {{ECC_Visibility,ECR_Block}});
}
bool UCollisionProfile::GetProfileTemplate(FName _ProfileName, FCollisionResponseTemplate& _ProfileData) const
{
    for (const auto& Profile : __Profiles) if (Profile.Name == _ProfileName) { _ProfileData = Profile; return true; }
    return false;
}
bool UCollisionProfile::SetProfile(const FCollisionResponseTemplate& _Profile)
{
    if (!ValidName(_Profile.Name) || _Profile.Name == FName(TEXT("Custom")) || _Profile.ObjectType < 0 || _Profile.ObjectType >= 32 || _Profile.CollisionEnabled < 0 || _Profile.CollisionEnabled > ECollisionEnabled::QueryAndProbe) return false;
    if (std::none_of(__Channels.begin(), __Channels.end(), [&](const auto& _Channel) { return !_Channel.bTraceType && _Channel.Channel == _Profile.ObjectType; })) return false;
    for (auto& Profile : __Profiles) if (Profile.Name == _Profile.Name)
    {
        const bool CanModify = Profile.bCanModify; Profile = _Profile; Profile.bCanModify = CanModify; ++__Revision; return true;
    }
    auto Copy = _Profile; Copy.bCanModify = true; __Profiles.push_back(Copy); ++__Revision; return true;
}
bool UCollisionProfile::RemoveProfile(FName _Name)
{
    auto Found = std::find_if(__Profiles.begin(), __Profiles.end(), [&](const auto& _Entry) { return _Entry.Name == _Name && _Entry.bCanModify; });
    if (Found == __Profiles.end()) return false;
    __Profiles.erase(Found); ++__Revision; return true;
}
bool UCollisionProfile::AddChannel(FName _Name, ECollisionResponse _Response, bool _bTraceType)
{
    if (!ValidName(_Name) || _Response < 0 || _Response >= ECR_MAX) return false;
    for (const auto& Channel : __Channels) if (Channel.Name == _Name) return false;
    for (int Index = ECC_GameTraceChannel1; Index <= ECC_GameTraceChannel18; ++Index)
    {
        if (std::any_of(__Channels.begin(), __Channels.end(), [&](const auto& _Channel) { return _Channel.Channel == Index; })) continue;
        __Channels.push_back({static_cast<ECollisionChannel>(Index), _Response, _bTraceType, false, _Name});
        for (auto& Profile : __Profiles) Profile.ResponseToChannels.SetResponse(static_cast<ECollisionChannel>(Index), _Response);
        ++__Revision; return true;
    }
    return false;
}
bool UCollisionProfile::EditChannel(ECollisionChannel _Channel, FName _Name, ECollisionResponse _Response)
{
    if (_Channel < ECC_GameTraceChannel1 || !ValidName(_Name) || _Response < 0 || _Response >= ECR_MAX) return false;
    for (const auto& Channel : __Channels) if (Channel.Channel != _Channel && Channel.Name == _Name) return false;
    for (auto& Channel : __Channels) if (Channel.Channel == _Channel)
    {
        for (auto& Profile : __Profiles) if (Profile.ResponseToChannels.GetResponse(_Channel) == Channel.DefaultResponse) Profile.ResponseToChannels.SetResponse(_Channel, _Response);
        Channel.Name = _Name; Channel.DefaultResponse = _Response; ++__Revision; return true;
    }
    return false;
}
bool UCollisionProfile::RemoveChannel(ECollisionChannel _Channel)
{
    if (_Channel < ECC_GameTraceChannel1 || _Channel > ECC_GameTraceChannel18) return false;
    const auto OldSize = __Channels.size();
    std::erase_if(__Channels, [&](const auto& _Entry) { return _Entry.Channel == _Channel; });
    if (OldSize == __Channels.size()) return false;
    for (auto& Profile : __Profiles)
    {
        if (Profile.ObjectType == _Channel) Profile.ObjectType = ECC_WorldStatic;
        Profile.ResponseToChannels.SetResponse(_Channel, ECR_Block);
    }
    ++__Revision; return true;
}
bool UCollisionProfile::SaveConfig(const std::filesystem::path& _Path) const
{
    std::error_code Error;
    if (!_Path.parent_path().empty()) std::filesystem::create_directories(_Path.parent_path(), Error);
    if (Error) return false;
    auto Temporary = _Path; Temporary += L".tmp";
    std::ofstream File(Temporary, std::ios::binary | std::ios::trunc);
    File << "UECollisionProfiles 2\n" << __Channels.size() << '\n';
    for (const auto& Channel : __Channels) File << int(Channel.Channel) << ' ' << int(Channel.DefaultResponse) << ' ' << Channel.bTraceType << ' ' << Channel.bStaticObject << ' ' << std::quoted(Channel.Name.ToString().ToUtf8()) << '\n';
    File << __Profiles.size() << '\n';
    for (const auto& Profile : __Profiles)
    {
        File << std::quoted(Profile.Name.ToString().ToUtf8()) << ' ' << int(Profile.CollisionEnabled) << ' ' << int(Profile.ObjectType) << ' ' << Profile.bCanModify;
        for (int Index = 0; Index < 32; ++Index) File << ' ' << int(Profile.ResponseToChannels.GetResponse(static_cast<ECollisionChannel>(Index)));
        File << ' ' << std::quoted(Profile.HelpMessage.ToUtf8()) << '\n';
    }
    File.flush(); if (!File) return false; File.close();
    return MoveFileExW(Temporary.c_str(), _Path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
}
bool UCollisionProfile::LoadConfig(const std::filesystem::path& _Path)
{
    std::ifstream File(_Path, std::ios::binary);
    std::string Magic; int Version = 0; size_t Count = 0;
    if (!(File >> Magic >> Version >> Count) || Magic != "UECollisionProfiles" || (Version != 1 && Version != 2) || Count < 8 || Count > 26) return false;
    UCollisionProfile Loaded;
    Loaded.__Channels.clear();
    std::set<int> Indices; std::set<std::string> Names;
    for (size_t Index = 0; Index < Count; ++Index)
    {
        int Channel, Response, Trace, Static; std::string Name;
        if (!(File >> Channel >> Response >> Trace >> Static >> std::quoted(Name)) || Name.empty() || Name.size() > 128 || Response < 0 || Response >= 3 || Trace < 0 || Trace > 1 || Static < 0 || Static > 1 || Channel < 0 || Channel > 31 || (Channel >= 8 && Channel < 14) || !Indices.insert(Channel).second || !Names.insert(Name).second) return false;
        Loaded.__Channels.push_back({static_cast<ECollisionChannel>(Channel), static_cast<ECollisionResponse>(Response), Trace != 0, Static != 0, FName(Name.c_str())});
    }
    UCollisionProfile Defaults;
    for (const auto& Builtin : Defaults.__Channels)
    {
        auto Found = std::find_if(Loaded.__Channels.begin(), Loaded.__Channels.end(), [&](const auto& _C) { return _C.Channel == Builtin.Channel; });
        if (Found == Loaded.__Channels.end() || Found->Name != Builtin.Name || Found->bTraceType != Builtin.bTraceType) return false;
    }
    if (!(File >> Count) || Count < 18 || Count > 1024) return false;
    Names.clear(); Loaded.__Profiles.clear();
    for (size_t Index = 0; Index < Count; ++Index)
    {
        FCollisionResponseTemplate Profile; std::string Name; int Enabled, Object, Modify;
        if (!(File >> std::quoted(Name) >> Enabled >> Object >> Modify) || Name.empty() || Name.size() > 128 || !Names.insert(Name).second || Enabled < 0 || Enabled > 5 || Object < 0 || Object >= 32 || Modify < 0 || Modify > 1) return false;
        Profile.Name = FName(Name.c_str()); Profile.CollisionEnabled = static_cast<ECollisionEnabled::Type>(Enabled); Profile.ObjectType = static_cast<ECollisionChannel>(Object);
        for (int C = 0; C < 32; ++C) { int Response; if (!(File >> Response) || Response < 0 || Response >= 3) return false; Profile.ResponseToChannels.SetResponse(static_cast<ECollisionChannel>(C), static_cast<ECollisionResponse>(Response)); }
        if (Version >= 2) { std::string Description; if (!(File >> std::quoted(Description)) || Description.size() > 4096) return false; Profile.HelpMessage = Description.c_str(); }
        if (!Loaded.SetProfile(Profile)) return false;
        FCollisionResponseTemplate Builtin;
        Loaded.__Profiles.back().bCanModify = !Defaults.GetProfileTemplate(Profile.Name, Builtin);
    }
    for (const auto& Builtin : Defaults.__Profiles) { FCollisionResponseTemplate Found; if (!Loaded.GetProfileTemplate(Builtin.Name, Found)) return false; }
    File >> std::ws; if (!File.eof()) return false;
    __Profiles = std::move(Loaded.__Profiles); __Channels = std::move(Loaded.__Channels); ++__Revision; return true;
}
