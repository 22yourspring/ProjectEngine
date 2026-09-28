#pragma once
#include "EngineTypes.h"
#include "NameTypes.h"
#include <vector>
#include <filesystem>

struct FCollisionResponseTemplate
{
    FName Name;
    ECollisionEnabled::Type CollisionEnabled = ECollisionEnabled::QueryAndPhysics;
    ECollisionChannel ObjectType = ECC_WorldDynamic;
    FCollisionResponseContainer ResponseToChannels;
    FString HelpMessage;
    bool bCanModify = true;
};
struct FCustomChannelSetup
{
    ECollisionChannel Channel = ECC_GameTraceChannel1;
    ECollisionResponse DefaultResponse = ECR_Block;
    bool bTraceType = false;
    bool bStaticObject = false;
    FName Name;
};
class ENGINE_API UCollisionProfile
{
public:
    static UCollisionProfile* Get();
    bool GetProfileTemplate(FName _ProfileName, FCollisionResponseTemplate& _ProfileData) const;
    const std::vector<FCollisionResponseTemplate>& GetProfiles() const { return __Profiles; }
    const std::vector<FCustomChannelSetup>& GetChannels() const { return __Channels; }
    bool SetProfile(const FCollisionResponseTemplate& _Profile);
    bool RemoveProfile(FName _Name);
    bool AddChannel(FName _Name, ECollisionResponse _Response, bool _bTraceType);
    bool EditChannel(ECollisionChannel _Channel, FName _Name, ECollisionResponse _Response);
    bool RemoveChannel(ECollisionChannel _Channel);
    bool LoadConfig(const std::filesystem::path& _Path);
    bool SaveConfig(const std::filesystem::path& _Path) const;
    uint64 GetRevision() const { return __Revision; }
    void ResetToDefaults();
private:
    UCollisionProfile();
    std::vector<FCollisionResponseTemplate> __Profiles;
    std::vector<FCustomChannelSetup> __Channels;
    uint64 __Revision = 0;
};
