#include "pch.h"
#include "WorldPersistence.h"
#include "Archive.h"
#include "World.h"
#include "Actor.h"
#include "PlayerStart.h"
#include "Character.h"
#include "MannequinPawn.h"
#include "InputComponent.h"
#include "BillboardComponent.h"
#include "AudioComponent.h"
#include "StaticMeshComponent.h"
#include "StaticMesh.h"
#include "Texture2D.h"
#include "SoundWave.h"
#include "PackageStore.h"
#include "MaterialInterface.h"
#include "MaterialInstanceDynamic.h"
#include "BoxComponent.h"
#include "SphereComponent.h"
#include "CapsuleComponent.h"
#include "CollisionProfile.h"
#include <fstream>
#include <sstream>
#include <map>
#include <cmath>

namespace
{
    struct FType
    {
        std::type_index __Type;
        FWorldPersistence::FActorFactory __Actor;
        FWorldPersistence::FComponentFactory __Component;
    };
    std::map<FString, FType>& Types()
    {
        static std::map<FString, FType> Value;
        return Value;
    }
    void Builtins()
    {
        static const bool Ready = []
        {
            FWorldPersistence::RegisterActor(TEXT("/Script/Engine.Actor"), typeid(AActor), [](UWorld* _World) { return _World->SpawnActor<AActor>(); });
            FWorldPersistence::RegisterActor(TEXT("/Script/Engine.PlayerStart"), typeid(APlayerStart), [](UWorld* _World) { return _World->SpawnActor<APlayerStart>(); });
            FWorldPersistence::RegisterActor(TEXT("/Script/Engine.Pawn"), typeid(APawn), [](UWorld* _World) { return _World->SpawnActor<APawn>(); });
            FWorldPersistence::RegisterActor(TEXT("/Script/Engine.Character"), typeid(ACharacter), [](UWorld* _World) { return _World->SpawnActor<ACharacter>(); });
            FWorldPersistence::RegisterActor(TEXT("/Script/Engine.MannequinPawn"), typeid(AMannequinPawn), [](UWorld* _World) { return _World->SpawnActor<AMannequinPawn>(); });
            FWorldPersistence::RegisterComponent(TEXT("/Script/Engine.ActorComponent"), typeid(UActorComponent), [](AActor* _Actor) { return _Actor->CreateInstanceComponent<UActorComponent>(); });
            FWorldPersistence::RegisterComponent(TEXT("/Script/Engine.BoxComponent"), typeid(UBoxComponent), [](AActor* _Actor) { return _Actor->CreateInstanceComponent<UBoxComponent>(); });
            FWorldPersistence::RegisterComponent(TEXT("/Script/Engine.SphereComponent"), typeid(USphereComponent), [](AActor* _Actor) { return _Actor->CreateInstanceComponent<USphereComponent>(); });
            FWorldPersistence::RegisterComponent(TEXT("/Script/Engine.CapsuleComponent"), typeid(UCapsuleComponent), [](AActor* _Actor) { return _Actor->CreateInstanceComponent<UCapsuleComponent>(); });
            FWorldPersistence::RegisterComponent(TEXT("/Script/Engine.SceneComponent"), typeid(USceneComponent), [](AActor* _Actor) { return _Actor->CreateInstanceComponent<USceneComponent>(); });
            FWorldPersistence::RegisterComponent(TEXT("/Script/Engine.PrimitiveComponent"), typeid(UPrimitiveComponent), [](AActor* _Actor) { return _Actor->CreateInstanceComponent<UPrimitiveComponent>(); });
            FWorldPersistence::RegisterComponent(TEXT("/Script/Engine.StaticMeshComponent"), typeid(UStaticMeshComponent), [](AActor* _Actor) { return _Actor->CreateInstanceComponent<UStaticMeshComponent>(); });
            FWorldPersistence::RegisterComponent(TEXT("/Script/Engine.BillboardComponent"), typeid(UBillboardComponent), [](AActor* _Actor) { return _Actor->CreateInstanceComponent<UBillboardComponent>(); });
            FWorldPersistence::RegisterComponent(TEXT("/Script/Engine.AudioComponent"), typeid(UAudioComponent), [](AActor* _Actor) { return _Actor->CreateInstanceComponent<UAudioComponent>(); });
            FWorldPersistence::RegisterComponent(TEXT("/Script/Engine.InputComponent"), typeid(UInputComponent), [](AActor* _Actor) { return _Actor->CreateInstanceComponent<UInputComponent>(); });
            return true;
        }();
    }
    FString ClassName(UObject* _Object)
    {
        for (const auto& [Name, Type] : Types()) if (Type.__Type == typeid(*_Object)) return Name;
        return {};
    }
    void Number(FArchive& _Archive, double& _Value)
    {
        _Archive.Serialize(&_Value, sizeof(_Value));
        if (!std::isfinite(_Value)) _Archive.SetError();
    }
    void Properties(UObject* _Object, FArchive& _Archive, FPackageStore* _Packages)
    {
        FTickFunction* Tick = nullptr;
        if (auto* Actor = dynamic_cast<AActor*>(_Object)) Tick = &Actor->PrimaryActorTick;
        if (auto* Component = dynamic_cast<UActorComponent*>(_Object)) Tick = &Component->PrimaryComponentTick;
        if (Tick)
        {
            uint32 Enabled = Tick->bCanEverTick, Group = static_cast<uint32>(Tick->TickGroup);
            _Archive.UInt32(Enabled); _Archive.UInt32(Group);
            if (Enabled > 1 || Group > 5) _Archive.SetError();
            if (_Archive.IsLoading()) { Tick->bCanEverTick = Enabled != 0; Tick->TickGroup = static_cast<ETickingGroup>(Group); }
        }
        if (auto* Scene = dynamic_cast<USceneComponent*>(_Object))
        {
            auto Position = Scene->GetRelativeLocation();
            Number(_Archive, Position.X); Number(_Archive, Position.Y); Number(_Archive, Position.Z);
            if (_Archive.IsLoading() && !_Archive.IsError()) Scene->SetRelativeLocation(Position);
        }
        if (auto* Input = dynamic_cast<UInputComponent*>(_Object))
        {
            _Archive.Serialize(&Input->__Priority, sizeof(Input->__Priority));
            uint32 Block = Input->__bBlockInput; _Archive.UInt32(Block);
            if (Block > 1) _Archive.SetError();
            if (_Archive.IsLoading()) Input->__bBlockInput = Block != 0;
        }
        if (auto* Billboard = dynamic_cast<UBillboardComponent*>(_Object))
        {
            FString Path = Billboard->GetSprite() ? Billboard->GetSprite()->GetPathName() : FString();
            _Archive.String(Path);
            uint32 Width = Billboard->GetSpriteWidth(), Height = Billboard->GetSpriteHeight();
            _Archive.UInt32(Width); _Archive.UInt32(Height);
            if (Width > 65536 || Height > 65536) _Archive.SetError();
            if (_Archive.IsLoading() && !_Archive.IsError())
            {
                FString Error;
                auto* Asset = Path.IsEmpty() ? nullptr : dynamic_cast<UTexture2D*>(_Packages->Load(Path, Error));
                if (!Path.IsEmpty() && !Asset) _Archive.SetError();
                Billboard->SetSprite(Asset); Billboard->SetSize(Width, Height);
            }
        }
        if (auto* Audio = dynamic_cast<UAudioComponent*>(_Object))
        {
            FString Path = Audio->GetSound() ? Audio->GetSound()->GetPathName() : FString();
            _Archive.String(Path);
            double Volume = Audio->GetVolumeMultiplier(), Pitch = Audio->GetPitchMultiplier();
            uint32 Looping = Audio->IsLooping();
            Number(_Archive, Volume); Number(_Archive, Pitch); _Archive.UInt32(Looping);
            if (Volume < 0 || Volume > 16 || Pitch < 0.1 || Pitch > 4 || Looping > 1) _Archive.SetError();
            if (_Archive.IsLoading() && !_Archive.IsError())
            {
                FString Error;
                auto* Asset = Path.IsEmpty() ? nullptr : dynamic_cast<USoundWave*>(_Packages->Load(Path, Error));
                if (!Path.IsEmpty() && !Asset) _Archive.SetError();
                Audio->SetSound(Asset); Audio->SetVolumeMultiplier(static_cast<float>(Volume));
                Audio->SetPitchMultiplier(static_cast<float>(Pitch)); Audio->SetLooping(Looping != 0);
            }
        }
        if (auto* Component = dynamic_cast<UStaticMeshComponent*>(_Object))
        {
            auto* Mesh = Component->GetStaticMesh();
            uint32 Present = Mesh != nullptr;
            _Archive.UInt32(Present);
            if (Present > 1) _Archive.SetError();
            if (Present)
            {
                uint32 Width = Mesh ? Mesh->GetWidth() : 0, Height = Mesh ? Mesh->GetHeight() : 0;
                FColor Color = Mesh ? Mesh->GetColor() : FColor{};
                _Archive.UInt32(Width); _Archive.UInt32(Height); _Archive.Serialize(&Color, sizeof(Color));
                if (Width > 65536 || Height > 65536) _Archive.SetError();
                if (_Archive.IsLoading() && !_Archive.IsError()) Component->RestoreMesh(Width, Height, Color);
            }
            else if (_Archive.IsLoading()) Component->SetStaticMesh(nullptr);
        }
        _Object->Serialize(_Archive);
        if (auto* Mesh = dynamic_cast<UMeshComponent*>(_Object); Mesh && (!_Archive.IsLoading() || _Archive.Remaining()))
        {
            uint32 Tag = 0x4D415431;
            _Archive.UInt32(Tag);
            if (Tag != 0x4D415431) { _Archive.SetError(); return; }
            auto* Material = Mesh->GetMaterial(0);
            while (auto* Dynamic = dynamic_cast<UMaterialInstanceDynamic*>(Material)) Material = Dynamic->GetParent();
            UObject* Asset = Material;
            if (_Archive.IsLoading()) _Archive.SetAssetResolver([&](const FString& _Path)
                { FString Error; return _Packages->Load(_Path, Error); });
            _Archive.AssetReference(Asset);
            if (_Archive.IsLoading())
            {
                Material = dynamic_cast<UMaterialInterface*>(Asset);
                if (Asset && !Material) _Archive.SetError();
                if (!_Archive.IsError()) Mesh->SetMaterial(0, Material);
            }
        }
        if (auto* Scene = dynamic_cast<USceneComponent*>(_Object); Scene && (!_Archive.IsLoading() || _Archive.Remaining()))
        {
            uint32 Tag = 0x434F4C31; _Archive.UInt32(Tag);
            if (Tag != 0x434F4C31) { _Archive.SetError(); return; }
            auto Rotation = Scene->GetRelativeRotationQuaternion(); auto Scale = Scene->GetRelativeScale3D();
            Number(_Archive, Rotation.X); Number(_Archive, Rotation.Y); Number(_Archive, Rotation.Z); Number(_Archive, Rotation.W);
            Number(_Archive, Scale.X); Number(_Archive, Scale.Y); Number(_Archive, Scale.Z);
            if (!Rotation.IsNormalized()) { _Archive.SetError(); return; }
            if (_Archive.IsLoading()) { Scene->SetRelativeRotation(Rotation); Scene->SetRelativeScale3D(Scale); }
            if (auto* Primitive = dynamic_cast<UPrimitiveComponent*>(Scene))
            {
                auto& Body = Primitive->BodyInstance;
                FString Profile = Body.GetCollisionProfileName().ToString(); _Archive.String(Profile);
                uint32 Enabled = Body.GetCollisionEnabled(), Object = Body.GetObjectType(), Mode = Body.GetDOFLock();
                uint32 Flags = uint32(Body.bSimulatePhysics) | uint32(Body.bEnableGravity) << 1 | uint32(Primitive->GetGenerateOverlapEvents()) << 2 | uint32(Body.bNotifyRigidBodyCollision) << 3 | uint32(Body.bUseCCD) << 4;
                const bool Locks[] = {Body.bLockXTranslation, Body.bLockYTranslation, Body.bLockZTranslation, Body.bLockXRotation, Body.bLockYRotation, Body.bLockZRotation};
                for (int Index = 0; Index < 6; ++Index) Flags |= uint32(Locks[Index]) << (Index + 5);
                _Archive.UInt32(Enabled); _Archive.UInt32(Object); _Archive.UInt32(Mode); _Archive.UInt32(Flags);
                if (Enabled > 5 || Object > 31 || Mode > EDOFMode::None || Flags > 2047) { _Archive.SetError(); return; }
                auto Normal = Body.CustomDOFPlaneNormal; Number(_Archive, Normal.X); Number(_Archive, Normal.Y); Number(_Archive, Normal.Z);
                auto Responses = Body.GetResponseToChannels();
                for (int Index = 0; Index < 32; ++Index)
                {
                    uint32 Response = Responses.GetResponse(static_cast<ECollisionChannel>(Index)); _Archive.UInt32(Response);
                    if (Response >= ECR_MAX) { _Archive.SetError(); return; }
                    Responses.SetResponse(static_cast<ECollisionChannel>(Index), static_cast<ECollisionResponse>(Response));
                }
                FVector Dimensions(1);
                if (auto* Box = dynamic_cast<UBoxComponent*>(Primitive)) Dimensions = Box->GetUnscaledBoxExtent();
                if (auto* Sphere = dynamic_cast<USphereComponent*>(Primitive)) Dimensions = FVector(Sphere->GetUnscaledSphereRadius(), 1, 1);
                if (auto* Capsule = dynamic_cast<UCapsuleComponent*>(Primitive)) Dimensions = FVector(Capsule->GetUnscaledCapsuleRadius(), Capsule->GetUnscaledCapsuleHalfHeight(), 1);
                Number(_Archive, Dimensions.X); Number(_Archive, Dimensions.Y); Number(_Archive, Dimensions.Z);
                if (Dimensions.GetMin() <= 0 || Dimensions.GetMax() > 1.e7) { _Archive.SetError(); return; }
                if (_Archive.IsLoading() && !_Archive.IsError())
                {
                    if (auto* Box = dynamic_cast<UBoxComponent*>(Primitive)) Box->SetBoxExtent(Dimensions, false);
                    if (auto* Sphere = dynamic_cast<USphereComponent*>(Primitive)) Sphere->SetSphereRadius(float(Dimensions.X), false);
                    if (auto* Capsule = dynamic_cast<UCapsuleComponent*>(Primitive)) Capsule->SetCapsuleSize(float(Dimensions.X), float(Dimensions.Y), false);
                    Primitive->SetCollisionEnabled(static_cast<ECollisionEnabled::Type>(Enabled));
                    Primitive->SetCollisionObjectType(static_cast<ECollisionChannel>(Object)); Primitive->SetCollisionResponseToChannels(Responses);
                    FCollisionResponseTemplate Found;
                    if (UCollisionProfile::Get()->GetProfileTemplate(FName(Profile), Found)) Primitive->SetCollisionProfileName(FName(Profile), false);
                    Body.bEnableGravity = (Flags & 2) != 0; Primitive->SetGenerateOverlapEvents((Flags & 4) != 0);
                    Body.bNotifyRigidBodyCollision = (Flags & 8) != 0; Body.bUseCCD = (Flags & 16) != 0;
                    Body.bLockXTranslation = (Flags & 32) != 0; Body.bLockYTranslation = (Flags & 64) != 0; Body.bLockZTranslation = (Flags & 128) != 0;
                    Body.bLockXRotation = (Flags & 256) != 0; Body.bLockYRotation = (Flags & 512) != 0; Body.bLockZRotation = (Flags & 1024) != 0;
                    Body.CustomDOFPlaneNormal = Normal; Body.SetDOFLock(static_cast<EDOFMode::Type>(Mode));
                    Body.SetInstanceSimulatePhysics((Flags & 1) != 0, false, true);
                }
                if (!_Archive.IsLoading() || _Archive.Remaining())
                {
                    uint32 PhysicsTag = 0x50485931; _Archive.UInt32(PhysicsTag);
                    double Linear = Primitive->GetLinearDamping(), Angular = Primitive->GetAngularDamping();
                    Number(_Archive, Linear); Number(_Archive, Angular);
                    if (PhysicsTag != 0x50485931 || Linear < 0 || Angular < 0 || Linear > 1.e6 || Angular > 1.e6) { _Archive.SetError(); return; }
                    if (_Archive.IsLoading() && !_Archive.IsError())
                    {
                        Primitive->SetLinearDamping(float(Linear)); Primitive->SetAngularDamping(float(Angular));
                    }
                }
            }
        }
    }
    struct FRecord
    {
        FString __Class;
        uint32 __Owner = 0, __Parent = 0, __Root = 0, __Default = 0;
        std::vector<uint8> __Payload;
    };
    bool Decode(const std::vector<uint8>& _Data, std::vector<FRecord>& _Records)
    {
        if (_Data.size() > 64 * 1024 * 1024) return false;
        std::istringstream Stream(std::string(_Data.begin(), _Data.end()), std::ios::binary);
        FArchive Archive(Stream, _Data.size());
        uint32 Count = 0; Archive.UInt32(Count);
        if (Count > 100000) return false;
        _Records.resize(Count);
        for (auto& Record : _Records)
        {
            Archive.String(Record.__Class); Archive.UInt32(Record.__Owner); Archive.UInt32(Record.__Parent);
            if (Record.__Class.StartsWith(TEXT("/Script/UE."), ESearchCase::CaseSensitive))
                Record.__Class = FString(TEXT("/Script/Engine.")) + Record.__Class.Mid(11);
            Archive.UInt32(Record.__Root); Archive.UInt32(Record.__Default); Archive.Bytes(Record.__Payload, 1024 * 1024);
            if (Archive.IsError() || Record.__Owner > Count || Record.__Parent > Count || Record.__Root > Count || Record.__Default > 1) return false;
        }
        return !Archive.IsError() && Archive.Remaining() == 0;
    }
}

void FWorldPersistence::RegisterActor(const FString& _Name, std::type_index _Type, FActorFactory _Factory)
{
    Types().insert_or_assign(_Name, FType{_Type, std::move(_Factory), {}});
}
void FWorldPersistence::UnregisterClass(const FString& _Name) { Types().erase(_Name); }
void FWorldPersistence::RegisterComponent(const FString& _Name, std::type_index _Type, FComponentFactory _Factory)
{
    Types().insert_or_assign(_Name, FType{_Type, {}, std::move(_Factory)});
}
std::vector<FString> FWorldPersistence::GetActorClasses()
{
    Builtins(); std::vector<FString> Result;
    for (const auto& [Name, Type] : Types()) if (Type.__Actor) Result.push_back(Name);
    return Result;
}
AActor* FWorldPersistence::SpawnActor(const FString& _Name, UWorld* _World)
{
    Builtins();
    const auto Name = _Name.StartsWith(TEXT("/Script/UE."), ESearchCase::CaseSensitive) ? FString(TEXT("/Script/Engine.")) + _Name.Mid(11) : _Name;
    const auto Found = Types().find(Name);
    return _World && Found != Types().end() && Found->second.__Actor ? Found->second.__Actor(_World) : nullptr;
}
std::unique_ptr<UWorld> FWorldPersistence::CreateWorld(const FString& _Name) { return std::make_unique<UWorld>(FName(*_Name)); }
bool FWorldPersistence::Capture(UWorld* _World, std::vector<uint8>& _Data, FString& _Error)
{
    Builtins();
    if (!_World) { _Error = TEXT("No world to save."); return false; }
    if (_World->GetLevels().size() != 1) { _Error = TEXT("Saving streaming levels is not supported yet."); return false; }
    std::vector<UObject*> Objects;
    std::map<UObject*, uint32> IDs;
    auto Add = [&](UObject* _Object) { Objects.push_back(_Object); IDs[_Object] = static_cast<uint32>(Objects.size()); };
    for (const auto& Actor : _World->GetPersistentLevel()->GetActors())
    {
        if (Actor->IsPendingDestroy()) continue;
        Add(Actor.get());
        for (const auto& Component : Actor->GetComponentStorage()) if (!Component->IsPendingDestroy()) Add(Component.get());
    }
    std::ostringstream Stream(std::ios::binary); FArchive Archive(Stream);
    uint32 Count = static_cast<uint32>(Objects.size()); Archive.UInt32(Count);
    if (Count > 100000) { _Error = TEXT("Too many objects in this level."); return false; }
    auto ID = [&](UObject* _Object) { if (!_Object) return uint32(0); auto It = IDs.find(_Object); if (It == IDs.end()) { Archive.SetError(); return uint32(0); } return It->second; };
    for (auto* Object : Objects)
    {
        const auto Name = ClassName(Object);
        if (Name.IsEmpty()) { _Error = FString("Unregistered save class: ") + typeid(*Object).name(); return false; }
        FString Class(Name); Archive.String(Class);
        uint32 Owner = 0, Parent = 0, Root = 0, Default = 0;
        if (auto* Actor = dynamic_cast<AActor*>(Object)) Root = ID(Actor->GetRootComponent());
        if (auto* Component = dynamic_cast<UActorComponent*>(Object))
        {
            Owner = ID(Component->GetOwner());
            const auto& Instances = Component->GetOwner()->GetInstanceComponents();
            Default = std::find(Instances.begin(), Instances.end(), Component) == Instances.end();
        }
        if (auto* Scene = dynamic_cast<USceneComponent*>(Object)) Parent = ID(Scene->GetAttachParent());
        Archive.UInt32(Owner); Archive.UInt32(Parent); Archive.UInt32(Root); Archive.UInt32(Default);
        std::ostringstream Payload(std::ios::binary); FArchive PayloadArchive(Payload);
        PayloadArchive.SetObjectReferences(ID, {});
        Properties(Object, PayloadArchive, nullptr);
        if (PayloadArchive.IsError()) { _Error = TEXT("An object has invalid properties."); return false; }
        const auto Text = Payload.str(); std::vector<uint8> Bytes(Text.begin(), Text.end());
        Archive.Bytes(Bytes, 1024 * 1024);
    }
    if (Archive.IsError() || Stream.str().size() > 64 * 1024 * 1024) { _Error = TEXT("Could not serialize the level or an attachment references another world."); return false; }
    const auto Text = Stream.str(); _Data.assign(Text.begin(), Text.end()); return true;
}
std::unique_ptr<UWorld> FWorldPersistence::Restore(const std::vector<uint8>& _Data, const FString& _Name, FPackageStore& _Packages, FString& _Error)
{
    Builtins(); std::vector<FRecord> Records;
    if (!Decode(_Data, Records)) { _Error = TEXT("Invalid level object data."); return {}; }
    auto World = std::make_unique<UWorld>(FName(*_Name));
    std::vector<UObject*> Objects(Records.size() + 1, nullptr);
    std::map<AActor*, size_t> DefaultIndices;
    for (size_t I = 0; I < Records.size(); ++I)
    {
        const auto& Record = Records[I]; auto Type = Types().find(Record.__Class);
        if (Type == Types().end()) { _Error = TEXT("Missing class: ") + Record.__Class; return {}; }
        if (!Record.__Owner)
        {
            if (!Type->second.__Actor || Record.__Default || Record.__Parent) { _Error = TEXT("Invalid actor record."); return {}; }
            Objects[I + 1] = Type->second.__Actor(World.get());
        }
        else
        {
            auto* Owner = dynamic_cast<AActor*>(Objects[Record.__Owner]);
            if (!Owner || !Type->second.__Component || Record.__Root) { _Error = TEXT("Invalid component owner."); return {}; }
            if (Record.__Default)
            {
                auto Index = DefaultIndices[Owner]++;
                const auto& Components = Owner->GetComponentStorage();
                if (Index >= Components.size() || std::type_index(typeid(*Components[Index])) != Type->second.__Type) { _Error = TEXT("Default component layout changed; level was not loaded."); return {}; }
                Objects[I + 1] = Components[Index].get();
            }
            else Objects[I + 1] = Type->second.__Component(Owner);
        }
    }
    for (size_t I = 0; I < Records.size(); ++I)
    {
        const auto& Record = Records[I];
        if (auto* Scene = dynamic_cast<USceneComponent*>(Objects[I + 1])) Scene->DetachFromComponent();
        if (auto* Actor = dynamic_cast<AActor*>(Objects[I + 1]))
        {
            if (Actor->GetComponents().size() != static_cast<size_t>(std::count_if(Records.begin(), Records.end(), [I](const FRecord& _Record) { return _Record.__Owner == I + 1; }))) { _Error = TEXT("Component layout changed."); return {}; }
            auto* Root = dynamic_cast<USceneComponent*>(Objects[Record.__Root]);
            if ((Record.__Root && !Root) || !Actor->SetRootComponent(Root)) { _Error = TEXT("Invalid root component."); return {}; }
        }
    }
    for (size_t I = 0; I < Records.size(); ++I)
    {
        const auto& Record = Records[I];
        if (Record.__Parent)
        {
            auto* Scene = dynamic_cast<USceneComponent*>(Objects[I + 1]);
            auto* Parent = dynamic_cast<USceneComponent*>(Objects[Record.__Parent]);
            if (!Scene || !Parent || !Scene->SetupAttachment(Parent)) { _Error = TEXT("Invalid or cyclic component attachment."); return {}; }
        }
    }
    for (size_t I = 0; I < Records.size(); ++I)
    {
        const auto& Bytes = Records[I].__Payload;
        std::istringstream Stream(std::string(Bytes.begin(), Bytes.end()), std::ios::binary); FArchive Archive(Stream, Bytes.size());
        Archive.SetObjectReferences({}, [&](uint32 _ID) { return _ID < Objects.size() ? Objects[_ID] : nullptr; });
        Properties(Objects[I + 1], Archive, &_Packages);
        if (Archive.IsError() || Archive.Remaining()) { _Error = TEXT("Invalid object properties or missing asset reference."); return {}; }
        Objects[I + 1]->PostLoad();
    }
    return World;
}
bool FWorldPersistence::Read(const std::filesystem::path& _File, std::vector<uint8>& _Data, int& _GameMode, FString& _Error)
{
    std::ifstream File(_File, std::ios::binary | std::ios::ate);
    if (!File || File.tellg() < 0 || File.tellg() > 64 * 1024 * 1024) { _Error = TEXT("Could not read the level file."); return false; }
    auto Size = static_cast<uint64>(File.tellg()); File.seekg(0);
    FArchive Archive(File, Size); char Magic[8] = {}; Archive.Serialize(Magic, 8);
    uint32 Version = 0, Mode = 0; Archive.UInt32(Version); Archive.UInt32(Mode);
    std::vector<uint8> Data; Archive.Bytes(Data, 64 * 1024 * 1024);
    std::vector<FRecord> Records;
    if (Archive.IsError() || Archive.Remaining() || std::memcmp(Magic, "PEWORLD\0", 8) || Version != 1 || Mode > 2 || !Decode(Data, Records)) { _Error = TEXT("Unsupported or corrupt level package."); return false; }
    _GameMode = static_cast<int>(Mode) - 1; _Data = std::move(Data); return true;
}
bool FWorldPersistence::Write(const std::filesystem::path& _File, const std::vector<uint8>& _Data, int _GameMode, FString& _Error)
{
    std::vector<FRecord> Records;
    if (_GameMode < -1 || _GameMode > 1 || !Decode(_Data, Records)) { _Error = TEXT("Invalid level data."); return false; }
    std::error_code Error; std::filesystem::create_directories(_File.parent_path(), Error);
    const auto Temporary = std::filesystem::path(_File.wstring() + L".tmp");
    std::ofstream File(Temporary, std::ios::binary | std::ios::trunc); FArchive Archive(File);
    char Magic[8] = {'P','E','W','O','R','L','D',0}; Archive.Serialize(Magic, 8);
    uint32 Version = 1, Mode = _GameMode + 1; Archive.UInt32(Version); Archive.UInt32(Mode);
    auto Data = _Data; Archive.Bytes(Data, 64 * 1024 * 1024); File.flush(); const bool Valid = !Archive.IsError() && File.good(); File.close();
    if (Valid && MoveFileExW(Temporary.c_str(), _File.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
    std::filesystem::remove(Temporary, Error); _Error = TEXT("Could not save the level; the previous file was preserved."); return false;
}
