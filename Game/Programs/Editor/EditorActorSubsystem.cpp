#include "framework.h"
#include "EditorActorSubsystem.h"
#include "UE/World.h"
#include "UE/BillboardComponent.h"
#include "UE/AudioComponent.h"
#include "UE/Texture2D.h"
#include "UE/SoundWave.h"
#include "UE/PackageStore.h"
#include <fstream>
#include <iomanip>
#include <cmath>

std::vector<AActor*> UEditorActorSubsystem::GetAllLevelActors(UWorld* _World) const
{
    std::vector<AActor*> Actors;
    if (_World)
        for (auto* Level : _World->GetLevels())
            for (const auto& Actor : Level->GetActors())
                if (!Actor->IsPendingDestroy()) Actors.push_back(Actor.get());
    return Actors;
}

AActor* UEditorActorSubsystem::SpawnActorFromObject(UWorld* _World, UObject* _Asset, const FVector& _Location)
{
    if (!_World || (!dynamic_cast<UTexture2D*>(_Asset) && !dynamic_cast<USoundWave*>(_Asset))) return nullptr;
    auto* Actor = _World->SpawnActor<AActor>();
    if (auto* Texture = dynamic_cast<UTexture2D*>(_Asset))
    {
        auto* Component = Actor->CreateInstanceComponent<UBillboardComponent>();
        Actor->SetRootComponent(Component);
        Component->SetSprite(Texture);
    }
    else
    {
        auto* Component = Actor->CreateInstanceComponent<UAudioComponent>();
        Actor->SetRootComponent(Component);
        Component->SetSound(static_cast<USoundWave*>(_Asset));
    }
    Actor->SetActorLocation(_Location);
    __PlacedActors.insert(Actor);
    return Actor;
}

void UEditorActorSubsystem::CaptureEditorActors(UWorld* _World)
{
    __Records.clear();
    for (auto* Actor : GetAllLevelActors(_World))
    {
        if (!__PlacedActors.count(Actor)) continue;
        FActorRecord Record;
        Record.__Location = Actor->GetActorLocation();
        for (auto* Component : Actor->GetComponents())
        {
            if (auto* Sprite = dynamic_cast<UBillboardComponent*>(Component))
                Record.__Textures.push_back(Sprite->GetSprite() ? Sprite->GetSprite()->GetPathName() : FString());
            if (auto* Audio = dynamic_cast<UAudioComponent*>(Component))
                Record.__Sounds.push_back(Audio->GetSound() ? Audio->GetSound()->GetPathName() : FString());
        }
        __Records.push_back(std::move(Record));
    }
}

void UEditorActorSubsystem::RestoreEditorActors(UWorld* _World, FPackageStore& _Packages)
{
    __PlacedActors.clear();
    if (!_World) return;
    for (const auto& Record : __Records)
    {
        auto* Actor = _World->SpawnActor<AActor>();
        auto* Root = Actor->CreateInstanceComponent<USceneComponent>();
        Actor->SetRootComponent(Root);
        FString Error;
        for (const auto& Path : Record.__Textures)
        {
            auto* Component = Actor->CreateInstanceComponent<UBillboardComponent>();
            Component->SetupAttachment(Root);
            Component->SetSprite(Path.IsEmpty() ? nullptr : dynamic_cast<UTexture2D*>(_Packages.Load(Path, Error)));
        }
        for (const auto& Path : Record.__Sounds)
        {
            auto* Component = Actor->CreateInstanceComponent<UAudioComponent>();
            Component->SetupAttachment(Root);
            Component->SetSound(Path.IsEmpty() ? nullptr : dynamic_cast<USoundWave*>(_Packages.Load(Path, Error)));
        }
        Actor->SetActorLocation(Record.__Location);
        __PlacedActors.insert(Actor);
    }
}

bool UEditorActorSubsystem::SaveEditorActors(const std::filesystem::path& _File) const
{
    std::error_code Error;
    std::filesystem::create_directories(_File.parent_path(), Error);
    const auto Temporary = std::filesystem::path(_File.wstring() + L".tmp");
    std::ofstream File(Temporary, std::ios::trunc);
    File << "EditorAssetActors 1\n" << std::setprecision(17);
    for (const auto& Record : __Records)
    {
        File << Record.__Location.X << ' ' << Record.__Location.Y << ' ' << Record.__Location.Z << ' '
            << Record.__Textures.size() << ' ' << Record.__Sounds.size();
        for (const auto& Path : Record.__Textures) File << ' ' << std::quoted(Path.ToUtf8());
        for (const auto& Path : Record.__Sounds) File << ' ' << std::quoted(Path.ToUtf8());
        File << '\n';
    }
    File.flush();
    const bool Valid = static_cast<bool>(File);
    File.close();
    if (Valid && MoveFileExW(Temporary.c_str(), _File.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
    std::filesystem::remove(Temporary, Error);
    return false;
}

void UEditorActorSubsystem::LoadEditorActors(const std::filesystem::path& _File)
{
    std::ifstream File(_File);
    std::string Header;
    int Version = 0;
    if (!(File >> Header >> Version) || Header != "EditorAssetActors" || Version != 1) return;
    std::vector<FActorRecord> Records;
    while (File >> std::ws && File.peek() != std::char_traits<char>::eof())
    {
        FActorRecord Record;
        size_t Textures = 0, Sounds = 0;
        if (!(File >> Record.__Location.X >> Record.__Location.Y >> Record.__Location.Z >> Textures >> Sounds) ||
            Textures > 64 || Sounds > 64 || Records.size() >= 10000 ||
            !std::isfinite(Record.__Location.X) || !std::isfinite(Record.__Location.Y) || !std::isfinite(Record.__Location.Z)) return;
        std::string Path;
        for (size_t I = 0; I < Textures + Sounds; ++I)
        {
            if (!(File >> std::quoted(Path)) || Path.size() > 4096) return;
            (I < Textures ? Record.__Textures : Record.__Sounds).emplace_back(Path);
        }
        Records.push_back(std::move(Record));
    }
    __Records = std::move(Records);
}

void UEditorActorSubsystem::RefreshAsset(UWorld* _World, UObject* _Asset) const
{
    for (auto* Actor : GetAllLevelActors(_World))
        for (auto* Component : Actor->GetComponents())
        {
            if (auto* Billboard = dynamic_cast<UBillboardComponent*>(Component))
            {
                if (Billboard->GetSprite() == _Asset) Billboard->SetSprite(Billboard->GetSprite());
            }
            else if (auto* Audio = dynamic_cast<UAudioComponent*>(Component))
            {
                if (Audio->GetSound() == _Asset)
                {
                    const bool Playing = Audio->IsPlaying();
                    Audio->Stop();
                    if (Playing) Audio->Play();
                }
            }
        }
}
