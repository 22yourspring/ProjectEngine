#pragma once
#include "Object.h"
#include <filesystem>
#include <functional>
#include <typeindex>

class AActor;
class UActorComponent;
class UWorld;
class FPackageStore;

class ENGINE_API FWorldPersistence
{
public:
    using FActorFactory = std::function<AActor*(UWorld*)>;
    using FComponentFactory = std::function<UActorComponent*(AActor*)>;
    static void RegisterActor(const FString& _Name, std::type_index _Type, FActorFactory _Factory);
    static void RegisterComponent(const FString& _Name, std::type_index _Type, FComponentFactory _Factory);
    static void UnregisterClass(const FString& _Name);
    static AActor* SpawnActor(const FString& _Name, UWorld* _World);
    static std::unique_ptr<UWorld> CreateWorld(const FString& _Name);
    static std::vector<FString> GetActorClasses();
    static bool Capture(UWorld* _World, std::vector<uint8>& _Data, FString& _Error);
    static std::unique_ptr<UWorld> Restore(const std::vector<uint8>& _Data, const FString& _Name, FPackageStore& _Packages, FString& _Error);
    static bool Read(const std::filesystem::path& _File, std::vector<uint8>& _Data, int& _GameMode, FString& _Error);
    static bool Write(const std::filesystem::path& _File, const std::vector<uint8>& _Data, int _GameMode, FString& _Error);
};
