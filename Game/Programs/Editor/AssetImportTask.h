#pragma once
#include "AssetImport.h"

class UAssetImportTask : public UObject
{
public:
    std::filesystem::path __Filename;
    FString __DestinationPath;
    bool __ReplaceExisting = false;
    bool __Automated = false;
    bool __Succeeded = false;
    FString __Error;
};

class UAssetToolsImpl : public UObject
{
public:
    static bool IsSupportedSource(const std::filesystem::path& _Source);
    static FString SanitizeObjectName(FString _Name);
    static bool ImportAssetTasks(FPackageStore& _Packages, std::vector<UAssetImportTask>& _Tasks);
};

using FAssetTools = UAssetToolsImpl;
