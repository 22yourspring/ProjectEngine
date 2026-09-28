#include "framework.h"
#include "EditorReimportHandler.h"
#include "UE/Texture2D.h"
#include "UE/SoundWave.h"
#include <cwctype>

bool FAssetTools::IsSupportedSource(const std::filesystem::path& _Source)
{
    const auto Extension = _Source.extension().wstring();
    return !_wcsicmp(Extension.c_str(), L".png") || !_wcsicmp(Extension.c_str(), L".wav");
}

FString FAssetTools::SanitizeObjectName(FString _Name)
{
    for (auto& Character : _Name)
        if (Character < 128 && !iswalnum(Character) && Character != L'_' && Character != L'-') Character = L'_';
    if (_Name.IsEmpty()) _Name = L"Asset";
    if (_Name.Len() > 128) _Name = _Name.Left(128);
    return _Name;
}

bool FAssetTools::ImportAssetTasks(FPackageStore& _Packages, std::vector<UAssetImportTask>& _Tasks)
{
    bool Success = true;
    for (auto& Task : _Tasks)
    {
        Task.__Error.Empty();
        if (Task.__ReplaceExisting)
            Task.__Succeeded = FReimportManager::Instance()->Reimport(_Packages, Task.__DestinationPath,
                Task.__Error, Task.__Filename) == EReimportResult::Succeeded;
        else
            Task.__Succeeded = ImportAsset(_Packages, Task.__Filename, Task.__DestinationPath, Task.__Error);
        Success = Task.__Succeeded && Success;
    }
    return Success;
}

bool UReimportTextureFactory::CanReimport(UObject* _Object) const { return dynamic_cast<UTexture2D*>(_Object) != nullptr; }
bool UReimportSoundFactory::CanReimport(UObject* _Object) const { return dynamic_cast<USoundWave*>(_Object) != nullptr; }
std::unique_ptr<UObject> UReimportTextureFactory::Reimport(const std::filesystem::path& _Source, FString& _Error)
{
    if (_wcsicmp(_Source.extension().c_str(), L".png")) { _Error = TEXT("A Texture2D requires a PNG source."); return {}; }
    return FactoryCreateFile(_Source, _Error);
}
std::unique_ptr<UObject> UReimportSoundFactory::Reimport(const std::filesystem::path& _Source, FString& _Error)
{
    if (_wcsicmp(_Source.extension().c_str(), L".wav")) { _Error = TEXT("A SoundWave requires a WAV source."); return {}; }
    return FactoryCreateFile(_Source, _Error);
}

FReimportManager* FReimportManager::Instance() { static FReimportManager Manager; return &Manager; }
bool FReimportManager::CanReimport(UObject* _Object) const
{
    return dynamic_cast<UTexture2D*>(_Object) || dynamic_cast<USoundWave*>(_Object);
}

std::filesystem::path FReimportManager::GetSourceFilename(FPackageStore& _Packages, const FString& _ObjectPath) const
{
    FString Source, Error, Package, Name;
    std::filesystem::path AssetFile;
    if (!_Packages.ReadImportSource(_ObjectPath, Source, Error) || Source.IsEmpty() ||
        !_Packages.Resolve(_ObjectPath, AssetFile, Package, Name)) return {};
    std::filesystem::path File(Source.ToWide());
    return (File.is_absolute() ? File : AssetFile.parent_path() / File).lexically_normal();
}

EReimportResult::Type FReimportManager::Reimport(FPackageStore& _Packages, const FString& _ObjectPath,
    FString& _Error, const std::filesystem::path& _NewSource)
{
    try
    {
        UObject* Existing = _Packages.Load(_ObjectPath, _Error);
        if (!Existing) return EReimportResult::Failed;
        const auto Source = _NewSource.empty() ? GetSourceFilename(_Packages, _ObjectPath) : _NewSource;
        if (Source.empty() || !std::filesystem::is_regular_file(Source))
        {
            _Error = TEXT("Source file is missing. Use Reimport With New File to select its new location.");
            return EReimportResult::Failed;
        }
        UReimportTextureFactory TextureFactory;
        UReimportSoundFactory SoundFactory;
        FReimportHandler* Handler = TextureFactory.CanReimport(Existing) ? static_cast<FReimportHandler*>(&TextureFactory) :
            SoundFactory.CanReimport(Existing) ? static_cast<FReimportHandler*>(&SoundFactory) : nullptr;
        if (!Handler) { _Error = TEXT("This asset type cannot be reimported."); return EReimportResult::Failed; }
        auto Replacement = Handler->Reimport(Source, _Error);
        if (!Replacement) return EReimportResult::Failed;
        std::filesystem::path File;
        FString Package, Name;
        if (!_Packages.Resolve(_ObjectPath, File, Package, Name)) return EReimportResult::Failed;
        std::error_code Error;
        auto Relative = std::filesystem::relative(std::filesystem::absolute(Source), File.parent_path(), Error);
        const FString StoredSource((Error || Relative.empty() ? std::filesystem::absolute(Source) : Relative).wstring());
        return _Packages.Save(_ObjectPath, *Replacement, StoredSource, _Error, true) ?
            EReimportResult::Succeeded : EReimportResult::Failed;
    }
    catch (const std::exception&) { _Error = TEXT("Reimport failed while reading the source file."); return EReimportResult::Failed; }
}
