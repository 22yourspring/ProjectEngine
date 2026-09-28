#pragma once

#include "UE/PackageStore.h"

class UFactory : public UObject
{
public:
    virtual std::unique_ptr<UObject> FactoryCreateFile(const std::filesystem::path& _Source, FString& _Error) = 0;
};

class UTextureFactory : public UFactory
{
public:
    std::unique_ptr<UObject> FactoryCreateFile(const std::filesystem::path& _Source, FString& _Error) override;
};

class USoundFactory : public UFactory
{
public:
    std::unique_ptr<UObject> FactoryCreateFile(const std::filesystem::path& _Source, FString& _Error) override;
};

bool ImportAsset(FPackageStore& _Packages, const std::filesystem::path& _Source,
    const FString& _ObjectPath, FString& _Error);
