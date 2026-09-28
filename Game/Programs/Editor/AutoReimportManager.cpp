#include "framework.h"
#include "AutoReimportManager.h"
#include "EditorReimportHandler.h"
#include <fstream>
#include <iomanip>
#include <set>
#include <algorithm>

UAutoReimportManager::~UAutoReimportManager()
{
    if (__Scan.valid()) __Scan.wait();
}

FString UAutoReimportManager::Key(const std::filesystem::path& _File, const FString& _ObjectPath)
{
    FString Path(std::filesystem::absolute(_File).lexically_normal().wstring());
    std::transform(Path.begin(), Path.end(), Path.begin(), towlower);
    return Path + TEXT("|") + _ObjectPath;
}

FString UAutoReimportManager::Stamp(const std::filesystem::path& _File)
{
    std::error_code Error;
    const auto Time = std::filesystem::last_write_time(_File, Error);
    if (Error) return {};
    const auto Size = std::filesystem::file_size(_File, Error);
    return Error ? FString() : FString::Printf(TEXT("%lld:%llu"), static_cast<long long>(Time.time_since_epoch().count()), static_cast<unsigned long long>(Size));
}

void UAutoReimportManager::Initialize(FPackageStore& _Packages, const std::filesystem::path& _Content,
    const std::filesystem::path& _Saved)
{
    __Packages = &_Packages;
    __Content = _Content;
    __SettingsFile = _Saved / L"Config/EditorLoadingSavingSettings.ini";
    __StateFile = _Saved / L"Config/AutoReimportState.txt";
    std::ifstream Settings(__SettingsFile);
    Settings >> __Settings.__MonitorContentDirectories >> __Settings.__AutoCreateAssets >>
        __Settings.__PromptBeforeAction >> __Settings.__AutoReimportThreshold;
    __Settings.__AutoReimportThreshold = (std::clamp)(__Settings.__AutoReimportThreshold, 0.5f, 30.0f);
    std::ifstream State(__StateFile);
    std::string File, Value;
    while (State >> std::quoted(File) >> std::quoted(Value)) __Seen[File] = Value;
}

void UAutoReimportManager::SaveSettings()
{
    std::error_code Error;
    std::filesystem::create_directories(__SettingsFile.parent_path(), Error);
    std::ofstream File(__SettingsFile);
    File << __Settings.__MonitorContentDirectories << ' ' << __Settings.__AutoCreateAssets << ' ' <<
        __Settings.__PromptBeforeAction << ' ' << __Settings.__AutoReimportThreshold;
}

void UAutoReimportManager::SaveState()
{
    std::error_code Error;
    std::filesystem::create_directories(__StateFile.parent_path(), Error);
    std::ofstream File(__StateFile);
    for (const auto& Entry : __Seen) File << std::quoted(Entry.first.ToUtf8()) << ' ' << std::quoted(Entry.second.ToUtf8()) << '\n';
}

std::vector<UAutoReimportManager::FSourceFile> UAutoReimportManager::Scan() const
{
    std::vector<FSourceFile> Result;
    std::vector<std::filesystem::path> Sources;
    std::set<FString> Imported;
    std::error_code Error;
    for (std::filesystem::recursive_directory_iterator It(__Content,
        std::filesystem::directory_options::skip_permission_denied, Error), End; It != End; It.increment(Error))
    {
        if (Error) { Error.clear(); continue; }
        if (!It->is_regular_file(Error)) continue;
        const auto File = It->path();
        if (FAssetTools::IsSupportedSource(File)) Sources.push_back(File);
        else if (!_wcsicmp(File.extension().c_str(), L".uasset"))
        {
            FString Path;
            if (!__Packages->GetObjectPath(File, Path)) continue;
            auto Source = FReimportManager::Instance()->GetSourceFilename(*__Packages, Path);
            if (Source.empty()) continue;
            FString Normal(std::filesystem::absolute(Source).lexically_normal().wstring());
            std::transform(Normal.begin(), Normal.end(), Normal.begin(), towlower);
            Imported.insert(Normal);
            const auto Value = Stamp(Source);
            if (!Value.IsEmpty()) Result.push_back({Source, Path, Value, true});
        }
    }
    for (const auto& Source : Sources)
    {
        FString Normal(std::filesystem::absolute(Source).lexically_normal().wstring());
        std::transform(Normal.begin(), Normal.end(), Normal.begin(), towlower);
        if (Imported.count(Normal)) continue;
        FString Path;
        const auto Asset = Source.parent_path() / (FAssetTools::SanitizeObjectName(Source.stem().wstring()) + L".uasset").ToWide();
        if (!__Packages->GetObjectPath(Asset, Path)) continue;
        const auto Value = Stamp(Source);
        if (!Value.IsEmpty()) Result.push_back({Source, Path, Value, false});
    }
    return Result;
}

std::vector<UAssetImportTask> UAutoReimportManager::Tick(double _Time)
{
    std::vector<UAssetImportTask> Tasks;
    if (!__Packages) return Tasks;
    if (__Scan.valid() && __Scan.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        std::vector<FSourceFile> Files;
        try { Files = __Scan.get(); } catch (const std::exception&) {}
        if (__Settings.__MonitorContentDirectories)
        {
            for (const auto& File : Files)
            {
                if (!File.__Existing && !__Settings.__AutoCreateAssets) continue;
                const auto FileKey = Key(File.__File, File.__ObjectPath);
                auto Seen = __Seen.find(FileKey);
                if (Seen != __Seen.end() && Seen->second == File.__Stamp) { __Pending.erase(FileKey); continue; }
                if (File.__Existing && Seen == __Seen.end())
                {
                    __Seen[FileKey] = File.__Stamp;
                    continue;
                }
                auto& Pending = __Pending[FileKey];
                if (Pending.__Stamp != File.__Stamp) { Pending = {File.__Stamp, _Time}; continue; }
                if (_Time - Pending.__Since < __Settings.__AutoReimportThreshold) continue;
                UAssetImportTask Task;
                Task.__Filename = File.__File;
                Task.__DestinationPath = File.__ObjectPath;
                Task.__ReplaceExisting = File.__Existing;
                Task.__Automated = true;
                Tasks.push_back(std::move(Task));
                __Seen[FileKey] = File.__Stamp;
                __Pending.erase(FileKey);
            }
            SaveState();
        }
    }
    if (__Settings.__MonitorContentDirectories && !__Scan.valid() && _Time >= __NextScan)
    {
        __NextScan = _Time + 1.0;
        __Scan = std::async(std::launch::async, [this] { return Scan(); });
    }
    return Tasks;
}

void UAutoReimportManager::Acknowledge(const UAssetImportTask& _Task)
{
    if (_Task.__Filename.empty()) return;
    __Seen[Key(_Task.__Filename, _Task.__DestinationPath)] = Stamp(_Task.__Filename);
    SaveState();
}
