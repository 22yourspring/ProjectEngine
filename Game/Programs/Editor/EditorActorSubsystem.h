#pragma once
#include "EditorSubsystem.h"
#include "UE/Math/Vector.h"
#include <filesystem>
#include <set>

class AActor;
class UWorld;
class FPackageStore;

class UEditorActorSubsystem : public UEditorSubsystem
{
public:
    std::vector<AActor*> GetAllLevelActors(UWorld* _World) const;
    AActor* SpawnActorFromObject(UWorld* _World, UObject* _Asset, const FVector& _Location);
    void RefreshAsset(UWorld* _World, UObject* _Asset) const;
    void CaptureEditorActors(UWorld* _World);
    void RestoreEditorActors(UWorld* _World, FPackageStore& _Packages);
    bool SaveEditorActors(const std::filesystem::path& _File) const;
    void LoadEditorActors(const std::filesystem::path& _File);

private:
    struct FActorRecord
    {
        FVector __Location;
        std::vector<FString> __Textures;
        std::vector<FString> __Sounds;
    };
    std::set<AActor*> __PlacedActors;
    std::vector<FActorRecord> __Records;
};
