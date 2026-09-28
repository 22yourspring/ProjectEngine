#pragma once
#include "Package.h"
#include <filesystem>
#include <mutex>
#include <map>
#include <set>

class ENGINE_API FPackageStore final
{
public:
    void Mount(const FString& _Root, const std::filesystem::path& _ContentDirectory);
    bool Resolve(const FString& _ObjectPath, std::filesystem::path& _File, FString& _PackageName, FString& _ObjectName) const;
    bool GetObjectPath(const std::filesystem::path& _File, FString& _ObjectPath) const;
    UObject* Load(const FString& _ObjectPath, FString& _Error);
    bool Save(const FString& _ObjectPath, UObject& _Object, const FString& _Source, FString& _Error, bool _ReplaceExisting = false);
    bool ReadImportSource(const FString& _ObjectPath, FString& _Source, FString& _Error, uint32* _Kind = nullptr) const;
    bool SaveDirtyPackages(FString& _Error);
    bool HasDirtyPackages() const;

private:
    struct FLoadedPackage
    {
        std::unique_ptr<UPackage> __Package;
        std::unique_ptr<UObject> __Object;
    };
    mutable std::recursive_mutex __Mutex;
    std::map<FString, std::filesystem::path> __Mounts;
    std::map<FString, FLoadedPackage> __Loaded;
    std::set<FString> __Loading;
};
