#pragma once
#include "AssetImportTask.h"

namespace EReimportResult
{
    enum Type { Succeeded, Failed, Cancelled };
}

class FReimportHandler
{
public:
    virtual ~FReimportHandler() = default;
    virtual bool CanReimport(UObject* _Object) const = 0;
    virtual std::unique_ptr<UObject> Reimport(const std::filesystem::path& _Source, FString& _Error) = 0;
};

class UReimportTextureFactory : public UTextureFactory, public FReimportHandler
{
public:
    bool CanReimport(UObject* _Object) const override;
    std::unique_ptr<UObject> Reimport(const std::filesystem::path& _Source, FString& _Error) override;
};

class UReimportSoundFactory : public USoundFactory, public FReimportHandler
{
public:
    bool CanReimport(UObject* _Object) const override;
    std::unique_ptr<UObject> Reimport(const std::filesystem::path& _Source, FString& _Error) override;
};

class FReimportManager
{
public:
    static FReimportManager* Instance();
    bool CanReimport(UObject* _Object) const;
    std::filesystem::path GetSourceFilename(FPackageStore& _Packages, const FString& _ObjectPath) const;
    EReimportResult::Type Reimport(FPackageStore& _Packages, const FString& _ObjectPath,
        FString& _Error, const std::filesystem::path& _NewSource = {});
};
