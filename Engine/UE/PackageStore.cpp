#include "pch.h"
#include "PackageStore.h"

bool FPackageStore::HasDirtyPackages() const
{
    std::lock_guard<std::recursive_mutex> Lock(__Mutex);
    for (const auto& Entry : __Loaded) if (Entry.second.__Package->IsDirty()) return true;
    return false;
}

bool FPackageStore::SaveDirtyPackages(FString& _Error)
{
    std::lock_guard<std::recursive_mutex> Lock(__Mutex);
    for (auto& [Name, Loaded] : __Loaded)
        if (Loaded.__Package->IsDirty())
        {
            if (!Save(Loaded.__Object->GetPathName(), *Loaded.__Object, Loaded.__Package->__ImportSource, _Error, true)) return false;
            Loaded.__Package->SetDirtyFlag(false);
        }
    return true;
}
#include "Archive.h"
#include "Texture2D.h"
#include "SoundWave.h"
#include "Material.h"
#include "MaterialInstanceConstant.h"
#include <fstream>
#include <cwctype>
#include <cstring>

namespace
{
    FString Lower(FString _Value)
    {
        for (auto& Character : _Value) Character = static_cast<wchar_t>(towlower(Character));
        return _Value;
    }

    bool IsWithin(const std::filesystem::path& _Root, const std::filesystem::path& _File)
    {
        const auto Root = std::filesystem::weakly_canonical(_Root);
        const auto File = std::filesystem::weakly_canonical(_File);
        auto Position = File.begin();
        for (const auto& Part : Root)
        {
            if (Position == File.end() || Lower(Part.wstring()) != Lower(Position->wstring())) return false;
            ++Position;
        }
        return Position != File.end();
    }

    bool ValidSegment(const FString& _Name)
    {
        if (_Name.IsEmpty() || _Name.Len() > 128) return false;
        for (const wchar_t Character : _Name)
            if (!(Character >= 128 || iswalnum(Character) || Character == L'_' || Character == L'-')) return false;
        return true;
    }
}

void FPackageStore::Mount(const FString& _Root, const std::filesystem::path& _ContentDirectory)
{
    std::lock_guard<std::recursive_mutex> Lock(__Mutex);
    if (_Root != TEXT("/Game/") && _Root != TEXT("/Engine/")) return;
    if (_ContentDirectory.empty() || !__Loaded.empty()) return;
    __Mounts[_Root] = std::filesystem::absolute(_ContentDirectory).lexically_normal();
}

bool FPackageStore::Resolve(const FString& _ObjectPath, std::filesystem::path& _File, FString& _PackageName, FString& _ObjectName) const
{
    std::lock_guard<std::recursive_mutex> Lock(__Mutex);
    try
    {
        const FString& Path = _ObjectPath;
        if (Path.Len() > 1024) return false;
        const int32 Dot = Path.Find(TEXT("."), ESearchCase::CaseSensitive);
        const FString Package = Dot == INDEX_NONE ? Path : Path.Left(Dot);
        const int32 Slash = Package.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
        if (Slash == INDEX_NONE) return false;
        const FString Name = Package.Mid(Slash + 1);
        if (!ValidSegment(Name) || (Dot != INDEX_NONE && Path.Mid(Dot + 1) != Name)) return false;
        for (const auto& Mount : __Mounts)
        {
            if (!Package.StartsWith(Mount.first, ESearchCase::CaseSensitive)) continue;
            const FString Relative = Package.Mid(Mount.first.Len());
            int32 Begin = 0;
            while (Begin < Relative.Len())
            {
                const int32 End = Relative.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Begin);
                if (!ValidSegment(Relative.Mid(Begin, End == INDEX_NONE ? INT32_MAX : End - Begin))) return false;
                if (End == INDEX_NONE) break;
                Begin = End + 1;
            }
            const auto File = Mount.second / (Relative + TEXT(".uasset")).ToWide();
            if (!IsWithin(Mount.second, File)) return false;
            _File = File;
            _PackageName = FString(Package);
            _ObjectName = FString(Name);
            return true;
        }
    }
    catch (const std::filesystem::filesystem_error&) {}
    return false;
}

bool FPackageStore::GetObjectPath(const std::filesystem::path& _File, FString& _ObjectPath) const
{
    std::lock_guard<std::recursive_mutex> Lock(__Mutex);
    try
    {
        if (Lower(_File.extension().wstring()) != L".uasset") return false;
        for (const auto& Mount : __Mounts)
        {
            if (!IsWithin(Mount.second, _File)) continue;
            auto Relative = std::filesystem::relative(_File, Mount.second);
            Relative.replace_extension();
            const FString Candidate(Mount.first + Relative.generic_wstring() + L"." + Relative.filename().wstring());
            std::filesystem::path Checked;
            FString Package, Name;
            if (!Resolve(Candidate, Checked, Package, Name)) return false;
            _ObjectPath = Candidate;
            return true;
        }
    }
    catch (const std::filesystem::filesystem_error&) {}
    return false;
}

UObject* FPackageStore::Load(const FString& _ObjectPath, FString& _Error)
{
    std::lock_guard<std::recursive_mutex> Lock(__Mutex);
    _Error.Empty();
    std::filesystem::path File;
    FString PackageName, ObjectName;
    if (!Resolve(_ObjectPath, File, PackageName, ObjectName)) { _Error = TEXT("Invalid asset path. Use /Game/Folder/Name.Name or /Engine/Folder/Name.Name."); return nullptr; }
    const auto Key = PackageName.ToLower();
    const auto Existing = __Loaded.find(Key);
    if (Existing != __Loaded.end()) return Existing->second.__Object.get();
    if (__Loading.size() >= 64 || !__Loading.insert(Key).second) { _Error = TEXT("Cyclic or excessive asset dependency."); return nullptr; }
    struct FLoadGuard
    {
        std::set<FString>& __Set;
        FString __Key;
        ~FLoadGuard() { __Set.erase(__Key); }
    } Guard{__Loading, Key};
    try
    {
        std::ifstream Input(File, std::ios::binary | std::ios::ate);
        if (!Input) { _Error = TEXT("Asset file could not be opened."); return nullptr; }
        const auto Size = Input.tellg();
        if (Size < 20 || Size > 257ull * 1024 * 1024) { _Error = TEXT("Invalid asset file size."); return nullptr; }
        Input.seekg(0);
        FArchive Archive(Input, static_cast<uint64>(Size));
        Archive.SetAssetResolver([&](const FString& _Path) { return Load(_Path, _Error); });
        char Magic[8] = {};
        uint32 Version = 0, Kind = 0;
        Archive.Serialize(Magic, 8);
        Archive.UInt32(Version);
        Archive.UInt32(Kind);
        if (std::memcmp(Magic, "PENGINEA", 8) || Version != 1 || (Kind != 1 && Kind != 2 && Kind != 4 && Kind != 5))
        {
            _Error = TEXT("Unsupported package format or version.");
            return nullptr;
        }
        FLoadedPackage Loaded;
        Loaded.__Package = std::make_unique<UPackage>();
        Loaded.__Package->__ObjectName = PackageName;
        Archive.String(Loaded.__Package->__ImportSource);
        if (Kind == 1) Loaded.__Object = std::make_unique<UTexture2D>();
        else if (Kind == 2) Loaded.__Object = std::make_unique<USoundWave>();
        else if (Kind == 4) Loaded.__Object = std::make_unique<UMaterial>();
        else Loaded.__Object = std::make_unique<UMaterialInstanceConstant>();
        Loaded.__Object->__ObjectName = ObjectName;
        Loaded.__Object->__Outer = Loaded.__Package.get();
        std::filesystem::path Source(Loaded.__Package->__ImportSource.ToWide());
        if (!Source.empty() && Source.is_relative()) Source = File.parent_path() / Source;
        const FString SourceFilename(Source.lexically_normal().wstring());
        if (auto* Texture = dynamic_cast<UTexture2D*>(Loaded.__Object.get())) Texture->GetAssetImportData()->UpdateFilenameOnly(SourceFilename);
        if (auto* Sound = dynamic_cast<USoundWave*>(Loaded.__Object.get())) Sound->GetAssetImportData()->UpdateFilenameOnly(SourceFilename);
        Loaded.__Object->Serialize(Archive);
        if (Archive.IsError() || Archive.Remaining()) { _Error = TEXT("Asset payload is invalid or truncated."); return nullptr; }
        Loaded.__Object->PostLoad();
        Loaded.__Package->SetDirtyFlag(false);
        return __Loaded.emplace(Key, std::move(Loaded)).first->second.__Object.get();
    }
    catch (const std::exception&) { _Error = TEXT("Asset load failed."); return nullptr; }
}

bool FPackageStore::Save(const FString& _ObjectPath, UObject& _Object, const FString& _Source, FString& _Error, bool _ReplaceExisting)
{
    std::lock_guard<std::recursive_mutex> Lock(__Mutex);
    _Error.Empty();
    std::filesystem::path File, Temporary;
    FString PackageName, ObjectName;
    if (!Resolve(_ObjectPath, File, PackageName, ObjectName)) { _Error = TEXT("Invalid destination asset path."); return false; }
    uint32 Kind = dynamic_cast<UTexture2D*>(&_Object) ? 1 : dynamic_cast<USoundWave*>(&_Object) ? 2 :
        typeid(_Object) == typeid(UMaterial) ? 4 : typeid(_Object) == typeid(UMaterialInstanceConstant) ? 5 : 0;
    if (!Kind) { _Error = TEXT("Only Texture2D, SoundWave, Material and MaterialInstanceConstant packages are supported."); return false; }
    try
    {
        UObject* Existing = nullptr;
        if (std::filesystem::exists(File))
        {
            if (!_ReplaceExisting) { _Error = TEXT("An asset with this name already exists. Choose a different name."); return false; }
            Existing = Load(_ObjectPath, _Error);
            if (!Existing) return false;
            if (typeid(*Existing) != typeid(_Object)) { _Error = TEXT("Cannot replace an asset with a different type."); return false; }
            if (Kind >= 4 && Existing != &_Object) { _Error = TEXT("Edit the loaded material asset before saving it."); return false; }
        }
        std::filesystem::create_directories(File.parent_path());
        wchar_t TempName[MAX_PATH] = {};
        if (!GetTempFileNameW(File.parent_path().c_str(), L"pea", 0, TempName)) { _Error = TEXT("Could not create the asset file."); return false; }
        Temporary = TempName;
        std::ofstream Output(Temporary, std::ios::binary | std::ios::trunc);
        FArchive Archive(Output);
        char Magic[8] = { 'P', 'E', 'N', 'G', 'I', 'N', 'E', 'A' };
        uint32 Version = 1;
        Archive.Serialize(Magic, 8);
        Archive.UInt32(Version);
        Archive.UInt32(Kind);
        FString Source = _Source;
        Archive.String(Source);
        _Object.Serialize(Archive);
        Output.flush();
        const bool Valid = !Archive.IsError() && static_cast<bool>(Output);
        Output.close();
        FString CommittedSource = _Source;
        std::filesystem::path SourceFilename(_Source.ToWide());
        if (!SourceFilename.empty() && SourceFilename.is_relative()) SourceFilename = File.parent_path() / SourceFilename;
        FString ResolvedSource(SourceFilename.lexically_normal().wstring());
        if (Valid && MoveFileExW(Temporary.c_str(), File.c_str(), MOVEFILE_WRITE_THROUGH |
            (_ReplaceExisting ? MOVEFILE_REPLACE_EXISTING : 0)))
        {
            if (Existing && Existing != &_Object)
            {
                if (auto* Texture = dynamic_cast<UTexture2D*>(Existing))
                {
                    auto& NewTexture = static_cast<UTexture2D&>(_Object);
                    std::swap(Texture->__SizeX, NewTexture.__SizeX);
                    std::swap(Texture->__SizeY, NewTexture.__SizeY);
                    Texture->__Pixels.swap(NewTexture.__Pixels);
                    Texture->GetAssetImportData()->UpdateFilenameOnly(std::move(ResolvedSource));
                }
                else if (auto* Sound = dynamic_cast<USoundWave*>(Existing))
                {
                    auto& NewSound = static_cast<USoundWave&>(_Object);
                    std::swap(Sound->__SampleRate, NewSound.__SampleRate);
                    std::swap(Sound->__NumChannels, NewSound.__NumChannels);
                    Sound->__PCMData.swap(NewSound.__PCMData);
                    Sound->GetAssetImportData()->UpdateFilenameOnly(std::move(ResolvedSource));
                }
                static_cast<UPackage*>(Existing->GetOuter())->__ImportSource = std::move(CommittedSource);
                Existing->PostLoad();
            }
            if (auto* Package = dynamic_cast<UPackage*>(_Object.GetOuter())) Package->SetDirtyFlag(false);
            return true;
        }
        _Error = TEXT("Could not save the asset. Check the data, destination and permissions.");
    }
    catch (const std::exception&) { _Error = TEXT("Asset save failed."); }
    if (!Temporary.empty()) { std::error_code Error; std::filesystem::remove(Temporary, Error); }
    return false;
}

bool FPackageStore::ReadImportSource(const FString& _ObjectPath, FString& _Source, FString& _Error, uint32* _Kind) const
{
    std::lock_guard<std::recursive_mutex> Lock(__Mutex);
    _Error.Empty();
    try
    {
        std::filesystem::path File;
        FString PackageName, ObjectName;
        if (!Resolve(_ObjectPath, File, PackageName, ObjectName)) return false;
        std::ifstream Input(File, std::ios::binary | std::ios::ate);
        if (!Input || Input.tellg() < 20) return false;
        const auto Size = Input.tellg();
        Input.seekg(0);
        FArchive Archive(Input, static_cast<uint64>(Size));
        char Magic[8] = {};
        uint32 Version = 0, Kind = 0;
        Archive.Serialize(Magic, 8);
        Archive.UInt32(Version);
        Archive.UInt32(Kind);
        if (std::memcmp(Magic, "PENGINEA", 8) || Version != 1 || (Kind != 1 && Kind != 2 && Kind != 4 && Kind != 5)) return false;
        Archive.String(_Source);
        if (_Kind) *_Kind = Kind;
        return !Archive.IsError();
    }
    catch (const std::exception&) { _Error = TEXT("Could not read asset import data."); return false; }
}
