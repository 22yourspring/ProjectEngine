#pragma once

#include "UE/UnrealString.h"
#include "AssetImportTask.h"
#include <future>
#include <map>

class UEditorLoadingSavingSettings : public UObject
{
public:
    bool __MonitorContentDirectories = true;
    bool __AutoCreateAssets = true;
    bool __PromptBeforeAction = true;
    float __AutoReimportThreshold = 2.0f;
};

class UAutoReimportManager : public UObject
{
public:
    ~UAutoReimportManager();
    void Initialize(FPackageStore& _Packages, const std::filesystem::path& _Content, const std::filesystem::path& _Saved);
    std::vector<UAssetImportTask> Tick(double _Time);
    void Acknowledge(const UAssetImportTask& _Task);
    void SaveSettings();
    UEditorLoadingSavingSettings& GetSettings() { return __Settings; }

private:
    struct FSourceFile
    {
        std::filesystem::path __File;
        FString __ObjectPath;
        FString __Stamp;
        bool __Existing = false;
    };
    struct FPendingFile { FString __Stamp; double __Since = 0; };
    std::vector<FSourceFile> Scan() const;
    static FString Key(const std::filesystem::path& _File, const FString& _ObjectPath);
    static FString Stamp(const std::filesystem::path& _File);
    void SaveState();
    FPackageStore* __Packages = nullptr;
    std::filesystem::path __Content, __SettingsFile, __StateFile;
    UEditorLoadingSavingSettings __Settings;
    std::future<std::vector<FSourceFile>> __Scan;
    std::map<FString, FString> __Seen;
    std::map<FString, FPendingFile> __Pending;
    double __NextScan = 0;
};
