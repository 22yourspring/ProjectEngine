#include "framework.h"
#include "EditorImGui.h"
#include "UE/BoxComponent.h"
#include "UE/SphereComponent.h"
#include "UE/CapsuleComponent.h"
#include "UE/MaterialInterface.h"
#include "EditorReimportHandler.h"
#include "AutoReimportManager.h"
#include "EditorActorSubsystem.h"
#include "UE/GameEngine.h"
#include "UE/World.h"
#include "UE/Pawn.h"
#include "UE/PathEngineSystem.h"
#include "UE/ResourceEngineSystem.h"
#include "UE/Texture2D.h"
#include "UE/SoundWave.h"
#include "UE/BillboardComponent.h"
#include "UE/AudioComponent.h"
#include "UE/StaticMeshComponent.h"
#include "UE/StaticMesh.h"
#include "UE/InputComponent.h"
#include <typeinfo>
#include <functional>
#include <cmath>
#include <commdlg.h>
#include <shellapi.h>
#include <algorithm>
#include <set>
#include "ThirdParty/WIL/include/wil/resource.h"

namespace
{
    FPackageStore* Packages()
    {
        auto* Resources = GEngine ? GEngine->GetEngineSystem<ResourceEngineSystem>() : nullptr;
        return Resources ? &Resources->GetPackageStore() : nullptr;
    }

    FString Label(const std::filesystem::path& _Path) { return FString(_Path.wstring()); }

    std::vector<std::filesystem::path> SelectSources(HWND _Window, bool _Multiple)
    {
        std::vector<wchar_t> Buffer(65536, 0);
        OPENFILENAMEW Dialog = {};
        Dialog.lStructSize = sizeof(Dialog);
        Dialog.hwndOwner = _Window;
        Dialog.lpstrTitle = L"Import Assets";
        Dialog.lpstrFilter = L"Texture and Sound (*.png;*.wav)\0*.png;*.wav\0PNG Texture\0*.png\0WAV Sound\0*.wav\0\0";
        Dialog.lpstrFile = Buffer.data();
        Dialog.nMaxFile = static_cast<DWORD>(Buffer.size());
        Dialog.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR |
            (_Multiple ? OFN_ALLOWMULTISELECT : 0);
        if (!GetOpenFileNameW(&Dialog)) return {};
        std::vector<std::filesystem::path> Files;
        const std::filesystem::path First(Buffer.data());
        const wchar_t* Next = Buffer.data() + wcslen(Buffer.data()) + 1;
        if (!*Next) Files.push_back(First);
        else while (*Next) { Files.push_back(First / Next); Next += wcslen(Next) + 1; }
        return Files;
    }

    bool Matches(const FString& _Text, const char* _Search)
    {
        return _Text.Contains(FString(_Search));
    }
}

void FEditorImGui::BeginAssetImport(const std::filesystem::path& _Destination)
{
    QueueImportFiles(SelectSources(__MainWindow, true), _Destination);
}

void FEditorImGui::QueueImportFiles(const std::vector<std::filesystem::path>& _Files,
    const std::filesystem::path& _Destination)
{
    auto* Store = Packages();
    if (!Store) return;
    std::set<FString> Seen;
    for (const auto& Task : __ImportTasks) Seen.insert(Task.__Filename.wstring());
    size_t Unsupported = 0;
    auto Add = [&](const std::filesystem::path& _File, const std::filesystem::path& _Folder)
    {
        if (!FAssetTools::IsSupportedSource(_File)) { ++Unsupported; return; }
        if (!Seen.insert(_File.wstring()).second) return;
        UAssetImportTask Task;
        Task.__Filename = _File;
        const auto Destination = _Folder / (FAssetTools::SanitizeObjectName(_File.stem().wstring()) + L".uasset").ToWide();
        if (!Store->GetObjectPath(Destination, Task.__DestinationPath)) { ++Unsupported; return; }
        __ImportTasks.push_back(std::move(Task));
    };
    for (const auto& File : _Files)
    {
        std::error_code Error;
        if (std::filesystem::is_directory(File, Error))
        {
            for (std::filesystem::recursive_directory_iterator It(File,
                std::filesystem::directory_options::skip_permission_denied, Error), End; It != End; It.increment(Error))
            {
                if (Error) { Error.clear(); continue; }
                if (It->is_regular_file(Error))
                {
                    const auto Relative = It->path().parent_path().lexically_relative(File);
                    auto Folder = _Destination / FAssetTools::SanitizeObjectName(File.filename().wstring()).ToWide();
                    for (const auto& Part : Relative)
                        if (Part != L".") Folder /= FAssetTools::SanitizeObjectName(Part.wstring()).ToWide();
                    Add(It->path(), Folder);
                }
            }
        }
        else Add(File, _Destination);
    }
    __ShowImport = !__ImportTasks.empty();
    if (Unsupported)
    {
        __AssetMessage = std::to_string(Unsupported) + " unsupported file(s) skipped. Supported: PNG, 16-bit PCM WAV.";
        __NotificationUntil = ImGui::GetTime() + 8;
    }
}

void FEditorImGui::HandleFileDrop(void* _Drop)
{
    HDROP Drop = static_cast<HDROP>(_Drop);
    auto FinishDrop = wil::scope_exit([Drop] { DragFinish(Drop); });
    std::vector<std::filesystem::path> Files;
    const UINT Count = DragQueryFileW(Drop, 0xffffffff, nullptr, 0);
    for (UINT Index = 0; Index < Count; ++Index)
    {
        std::vector<wchar_t> File(DragQueryFileW(Drop, Index, nullptr, 0) + 1, 0);
        DragQueryFileW(Drop, Index, File.data(), static_cast<UINT>(File.size()));
        Files.emplace_back(File.data());
    }
    if (__ContentSource == EContentSource::ProjectClasses || __ContentSource == EContentSource::EngineClasses)
    {
        __AssetMessage = "Select a Content folder before importing source files.";
        __NotificationUntil = ImGui::GetTime() + 5;
        return;
    }
    auto* Paths = GEngine ? GEngine->GetEngineSystem<PathEngineSystem>() : nullptr;
    if (!Paths) return;
    auto Destination = __ContentBrowserSelection.empty() ? Paths->GetProjectContentDirectory() : __ContentBrowserSelection;
    POINT Cursor;
    GetCursorPos(&Cursor);
    ScreenToClient(__MainWindow, &Cursor);
    float SmallestArea = FLT_MAX;
    for (const auto& Target : __DropTargets)
        if (Cursor.x >= Target.__Min.x && Cursor.x < Target.__Max.x &&
            Cursor.y >= Target.__Min.y && Cursor.y < Target.__Max.y)
        {
            const float Area = (Target.__Max.x - Target.__Min.x) * (Target.__Max.y - Target.__Min.y);
            if (Area < SmallestArea) { SmallestArea = Area; Destination = Target.__Directory; }
        }
    __ContentDrawerOpen = true;
    __ContentDrawerPinned = true;
    QueueImportFiles(Files, Destination);
}

void FEditorImGui::RunImportTasks(std::vector<UAssetImportTask>& _Tasks)
{
    auto* Store = Packages();
    auto* Engine = dynamic_cast<UGameEngine*>(GEngine);
    if (!Store || !Engine) return;
    size_t Succeeded = 0;
    FString Errors;
    Engine->WithWorld([&](UWorld* _World)
    {
        FAssetTools::ImportAssetTasks(*Store, _Tasks);
        for (const auto& Task : _Tasks)
        {
            if (Task.__Succeeded)
            {
                ++Succeeded;
                FString Error;
                UObject* Asset = Store->Load(Task.__DestinationPath, Error);
                if (__EditorActorSubsystem) __EditorActorSubsystem->RefreshAsset(_World, Asset);
                std::filesystem::path File;
                FString Package, Name;
                if (Store->Resolve(Task.__DestinationPath, File, Package, Name))
                {
                    const auto Thumbnail = __AssetThumbnails.find(File.wstring());
                    if (Thumbnail != __AssetThumbnails.end())
                    { __RetiredTextures.push_back(Thumbnail->second); __AssetThumbnails.erase(Thumbnail); }
                    __ContentBrowserFocusedPath = File;
                    if (__PreviewAsset == Asset) OpenAsset(File);
                }
            }
            else Errors += "\n" + Label(Task.__Filename.filename()) + ": " + Task.__Error;
            if (__AutoReimport) __AutoReimport->Acknowledge(Task);
        }
    });
    __NextAssetRefresh = 0;
    __AssetMessage = "Imported " + std::to_string(Succeeded) + " / " + std::to_string(_Tasks.size()) + " assets." + Errors;
    __NotificationUntil = ImGui::GetTime() + (Errors.IsEmpty() ? 5 : 15);
}

void FEditorImGui::ReimportAsset(const std::filesystem::path& _File, bool _WithNewFile)
{
    auto* Store = Packages();
    if (!Store) return;
    UAssetImportTask Task;
    if (!Store->GetObjectPath(_File, Task.__DestinationPath)) return;
    Task.__ReplaceExisting = true;
    if (_WithNewFile)
    {
        const auto Files = SelectSources(__MainWindow, false);
        if (Files.empty()) return;
        Task.__Filename = Files.front();
    }
    else Task.__Filename = FReimportManager::Instance()->GetSourceFilename(*Store, Task.__DestinationPath);
    std::vector<UAssetImportTask> Tasks;
    Tasks.push_back(std::move(Task));
    RunImportTasks(Tasks);
}

void FEditorImGui::TickAssetWorkflow()
{
    __RetiredTextures.clear();
    auto* Store = Packages();
    auto* Paths = GEngine ? GEngine->GetEngineSystem<PathEngineSystem>() : nullptr;
    if (!Store || !Paths) return;
    if (!__EditorActorSubsystem)
    {
        __EditorActorSubsystem = std::make_unique<UEditorActorSubsystem>();
        __EditorActorSubsystem->Initialize();
        __EditorActorSubsystem->LoadEditorActors(Paths->GetProjectSavedDirectory() / L"Config/EditorAssetActors.txt");
    }
    if (auto* Engine = dynamic_cast<UGameEngine*>(GEngine))
        Engine->WithWorld([&](UWorld* _World)
        {
            if (Engine->GetWorldGeneration() != __EditorWorldGeneration)
            {
                if (!__ProjectSessionReady) __EditorActorSubsystem->RestoreEditorActors(_World, *Store);
                __EditorWorldGeneration = Engine->GetWorldGeneration();
                __WaitingForWorldChange = false;
                __SelectedActor = nullptr;
                __DetailsActor = nullptr;
                __SelectedComponent = nullptr;
            }
            else if (!__ProjectSessionReady && !__IsPlaying && !__WaitingForWorldChange) __EditorActorSubsystem->CaptureEditorActors(_World);
        });
    if (!__AutoReimport)
    {
        __AutoReimport = std::make_unique<UAutoReimportManager>();
        __AutoReimport->Initialize(*Store, Paths->GetProjectContentDirectory(), Paths->GetProjectSavedDirectory());
    }
    auto Tasks = __AutoReimport->Tick(ImGui::GetTime());
    if (!Tasks.empty())
    {
        if (__AutoReimport->GetSettings().__PromptBeforeAction)
        {
            for (auto& Task : Tasks)
            {
                const bool AlreadyQueued = std::any_of(__ImportTasks.begin(), __ImportTasks.end(), [&](const auto& _Pending)
                    { return _Pending.__DestinationPath == Task.__DestinationPath; });
                if (!AlreadyQueued) __ImportTasks.push_back(std::move(Task));
            }
            __ShowImport = true;
        }
        else RunImportTasks(Tasks);
    }
    if (ImGui::GetTime() >= __NextAssetRefresh)
    {
        __NextAssetRefresh = ImGui::GetTime() + 2;
        __AssetFiles.clear();
        __AssetKinds.clear();
        std::error_code Error;
        for (const auto& ContentRoot : {Paths->GetProjectContentDirectory(), Paths->GetEngineContentDirectory()})
        for (std::filesystem::recursive_directory_iterator It(ContentRoot,
            std::filesystem::directory_options::skip_permission_denied, Error), End; It != End; It.increment(Error))
        {
            if (Error) { Error.clear(); continue; }
            if (It->is_regular_file(Error) && !_wcsicmp(It->path().extension().c_str(), L".uasset"))
            {
                __AssetFiles.push_back(It->path());
                FString Path, Source, ReadError;
                uint32 Kind = 0;
                if (Store->GetObjectPath(It->path(), Path) && Store->ReadImportSource(Path, Source, ReadError, &Kind))
                    __AssetKinds[It->path().wstring()] = Kind;
            }
            else if (It->is_regular_file(Error) && IsEditorMap(It->path()))
                __AssetKinds[It->path().wstring()] = 3;
        }
        std::sort(__AssetFiles.begin(), __AssetFiles.end());
    }
}

void FEditorImGui::DrawAssetContextMenu(const std::filesystem::path& _File)
{
    if (ImGui::BeginPopupContextItem("AssetActions"))
    {
        __ContentBrowserFocusedPath = _File;
        if (ImGui::MenuItem("Edit")) OpenAsset(_File);
        ImGui::Separator();
        const auto Kind = __AssetKinds.find(_File.wstring());
        const bool Material = Kind != __AssetKinds.end() && (Kind->second == 4 || Kind->second == 5);
        if (Material && ImGui::MenuItem("Create Material Instance"))
        {
            FString Path, Error;
            if (Packages() && Packages()->GetObjectPath(_File, Path))
            {
                __CreateMaterialParent = Packages()->Load(Path, Error);
                __MaterialDirectory = _File.parent_path();
                strcpy_s(__MaterialName, "MI_NewMaterial");
                __ShowMaterialCreate = true;
            }
        }
        ImGui::BeginDisabled(Material);
        if (ImGui::MenuItem("Reimport")) ReimportAsset(_File, false);
        if (ImGui::MenuItem("Reimport With New File...")) ReimportAsset(_File, true);
        ImGui::EndDisabled();
        ImGui::Separator();
        if (ImGui::MenuItem("Copy Reference"))
        {
            FString Path;
            if (Packages() && Packages()->GetObjectPath(_File, Path)) ImGui::SetClipboardText(Path.ToUtf8().c_str());
        }
        if (ImGui::MenuItem("Show in Explorer"))
        {
            const auto Arguments = L"/select,\"" + _File.wstring() + L"\"";
            ShellExecuteW(__MainWindow, L"open", L"explorer.exe", Arguments.c_str(), nullptr, SW_SHOWNORMAL);
        }
        ImGui::EndPopup();
    }
}

void FEditorImGui::DrawAssetDragSource(const std::filesystem::path& _File)
{
    if (ImGui::BeginDragDropSource())
    {
        const auto Path = _File.wstring();
        ImGui::SetDragDropPayload("ASSET_PATH", Path.c_str(), (Path.size() + 1) * sizeof(wchar_t));
        ImGui::TextUnformatted(Label(_File.stem()).ToUtf8().c_str());
        ImGui::EndDragDropSource();
    }
}

ID3D11ShaderResourceView* FEditorImGui::GetAssetThumbnail(const std::filesystem::path& _File)
{
    auto Found = __AssetThumbnails.find(_File.wstring());
    if (Found != __AssetThumbnails.end()) return Found->second.Get();
    if (__AssetThumbnails.size() >= 128) return nullptr;
    const auto Kind = __AssetKinds.find(_File.wstring());
    if (Kind == __AssetKinds.end() || Kind->second != 1) return nullptr;
    auto* Store = Packages();
    if (!Store) return nullptr;
    FString Path, Error;
    if (!Store->GetObjectPath(_File, Path)) return nullptr;
    auto* Texture = dynamic_cast<UTexture2D*>(Store->Load(Path, Error));
    if (!Texture) return nullptr;
    D3D11_TEXTURE2D_DESC Desc = {};
    Desc.Width = Texture->GetSizeX(); Desc.Height = Texture->GetSizeY();
    Desc.MipLevels = Desc.ArraySize = 1; Desc.SampleDesc.Count = 1;
    Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; Desc.Usage = D3D11_USAGE_IMMUTABLE;
    Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA Data = {Texture->GetPixels().data(), Desc.Width * 4, 0};
    Microsoft::WRL::ComPtr<ID3D11Texture2D> Resource;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> View;
    if (FAILED(__Device->CreateTexture2D(&Desc, &Data, &Resource)) ||
        FAILED(__Device->CreateShaderResourceView(Resource.Get(), nullptr, &View))) return nullptr;
    auto* Result = View.Get();
    __AssetThumbnails[_File.wstring()] = std::move(View);
    return Result;
}

bool FEditorImGui::DrawAssetProperty(const char* _Label, UObject*& _Asset, uint32 _Type)
{
    bool Changed = false;
    auto* Store = Packages();
    if (!Store) return false;
    auto Accept = [&](UObject* _Object)
    {
        return _Type == 1 ? dynamic_cast<UTexture2D*>(_Object) != nullptr :
            _Type == 4 ? dynamic_cast<UMaterialInterface*>(_Object) != nullptr : dynamic_cast<USoundWave*>(_Object) != nullptr;
    };
    ImGui::PushID(_Label);
    ImGui::TextUnformatted(_Label);
    ImGui::SameLine(110);
    const auto Current = _Asset ? _Asset->GetName() : FString("None");
    if (ImGui::Button(Current.ToUtf8().c_str(), ImVec2(160, 28))) ImGui::OpenPopup("AssetPicker");
    auto Assign = [&](const std::filesystem::path& _File)
    {
        FString Path, Error;
        if (!Store->GetObjectPath(_File, Path)) return;
        auto* Object = Store->Load(Path, Error);
        if (Accept(Object))
        { _Asset = Object; Changed = true; }
        else { __AssetMessage = _Type == 1 ? "Select a Texture2D asset." : _Type == 4 ? "Select a Material asset." : "Select a SoundWave asset."; __NotificationUntil = ImGui::GetTime() + 5; }
    };
    if (ImGui::BeginDragDropTarget())
    {
        if (const auto* Payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
            Assign(static_cast<const wchar_t*>(Payload->Data));
        ImGui::EndDragDropTarget();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("<-")) Assign(__ContentBrowserFocusedPath);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Use Selected Asset from Content Browser");
    ImGui::SameLine();
    if (ImGui::SmallButton("Browse") && _Asset)
    {
        std::filesystem::path File; FString Package, Name;
        if (Store->Resolve(_Asset->GetPathName(), File, Package, Name))
        {
            __ContentBrowserSelection = File.parent_path(); __ContentBrowserFocusedPath = File;
            if (auto* Paths = GEngine ? GEngine->GetEngineSystem<PathEngineSystem>() : nullptr)
            {
                const auto Relative = File.lexically_relative(Paths->GetEngineContentDirectory());
                const bool EngineContent = !Relative.empty() && *Relative.begin() != L"..";
                __ContentSource = EngineContent ? EContentSource::EngineContent : EContentSource::ProjectContent;
                if (EngineContent) __ShowEngineContent = true;
            }
            __ContentDrawerOpen = __ContentDrawerPinned = true;
        }
    }
    ImGui::SetNextWindowSize(ImVec2(380, 400), ImGuiCond_Appearing);
    if (ImGui::BeginPopup("AssetPicker"))
    {
        ImGui::InputTextWithHint("##Search", _Type == 1 ? "Search Texture2D Assets" : _Type == 4 ? "Search Material Assets" : "Search SoundWave Assets", __PickerSearch, sizeof(__PickerSearch));
        if (ImGui::Selectable("None", !_Asset)) { _Asset = nullptr; Changed = true; ImGui::CloseCurrentPopup(); }
        for (const auto& File : __AssetFiles)
        {
            if (!Matches(Label(File), __PickerSearch)) continue;
            const auto Kind = __AssetKinds.find(File.wstring());
            if (Kind == __AssetKinds.end() || (_Type == 4 ? (Kind->second != 4 && Kind->second != 5) : Kind->second != (_Type == 1 ? 1u : 2u))) continue;
            FString Path, Error;
            if (!Store->GetObjectPath(File, Path)) continue;
            auto* Object = Store->Load(Path, Error);
            if (!Accept(Object)) continue;
            if (ImGui::Selectable(Path.ToUtf8().c_str(), Object == _Asset))
            { _Asset = Object; Changed = true; ImGui::CloseCurrentPopup(); }
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();
    return Changed;
}

void FEditorImGui::PlaceAsset(const std::filesystem::path& _File, float _X, float _Y)
{
    auto* Engine = dynamic_cast<UGameEngine*>(GEngine);
    auto* Store = Packages();
    if (!Engine || !Store) return;
    FString Path, Error;
    if (!Store->GetObjectPath(_File, Path)) return;
    auto* Asset = Store->Load(Path, Error);
    Engine->WithWorld([&](UWorld* _World)
    {
        if (__EditorActorSubsystem) __SelectedActor = __EditorActorSubsystem->SpawnActorFromObject(_World, Asset, FVector(_X, _Y, 0));
    });
}

void FEditorImGui::DrawViewportAssetDrop(const ImVec2& _Position, const ImVec2& _Size)
{
    if (ImGui::BeginDragDropTarget())
    {
        if (const auto* Payload = ImGui::AcceptDragDropPayload("NATIVE_ACTOR_CLASS"))
        {
            const auto Mouse = ImGui::GetMousePos();
            PlaceNativeActor(FString(static_cast<const wchar_t*>(Payload->Data)),
                (std::clamp)(Mouse.x - _Position.x, 0.0f, _Size.x),
                (std::clamp)(Mouse.y - _Position.y, 0.0f, _Size.y));
        }
        if (const auto* Payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
        {
            const auto Mouse = ImGui::GetMousePos();
            PlaceAsset(static_cast<const wchar_t*>(Payload->Data),
                (std::clamp)(Mouse.x - _Position.x, 0.0f, _Size.x),
                (std::clamp)(Mouse.y - _Position.y, 0.0f, _Size.y));
        }
        ImGui::EndDragDropTarget();
    }
}

void FEditorImGui::DrawAssetPreferences()
{
    if (!__ShowAssetPreferences || !__AutoReimport) return;
    ImGui::SetNextWindowSize(ImVec2(680, 370), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Editor Preferences", &__ShowAssetPreferences))
    {
        ImGui::TextUnformatted("General / Loading & Saving");
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Startup and Auto Save", ImGuiTreeNodeFlags_DefaultOpen))
        {
            bool Changed = ImGui::Checkbox("Load last opened level", &__LoadLastLevel);
            Changed |= ImGui::Checkbox("Enable Auto Save", &__AutoSaveLevels);
            Changed |= ImGui::SliderInt("Auto Save Interval (minutes)", &__AutoSaveMinutes, 1, 60);
            if (Changed) SaveSessionSettings();
            ImGui::TextWrapped("Auto Save creates a recovery copy. Use Save to update the level package.");
        }
        if (ImGui::CollapsingHeader("Auto Reimport", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto& Settings = __AutoReimport->GetSettings();
            bool Changed = ImGui::Checkbox("Monitor Content Directories", &Settings.__MonitorContentDirectories);
            Changed |= ImGui::Checkbox("Auto Create Assets", &Settings.__AutoCreateAssets);
            Changed |= ImGui::Checkbox("Prompt Before Action", &Settings.__PromptBeforeAction);
            Changed |= ImGui::SliderFloat("Import Threshold Time", &Settings.__AutoReimportThreshold, 0.5f, 30.0f, "%.1f s");
            ImGui::TextUnformatted("Directories to Monitor: /Game/");
            ImGui::TextWrapped("Original source files of imported assets are also monitored. PNG textures and 16-bit PCM WAV sounds are supported.");
            if (Changed) __AutoReimport->SaveSettings();
        }
    }
    ImGui::End();
}

void FEditorImGui::DrawAssetWindows()
{
    DrawAssetPreferences();
    if (__ShowImport && !__ImportTasks.empty()) ImGui::OpenPopup("Import Assets");
    ImGui::SetNextWindowSize(ImVec2(780, 440), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Import Assets", nullptr, ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::TextUnformatted("Import Settings");
        ImGui::TextWrapped("Import the following source files into the project? Existing assets are preserved unless Replace Existing is selected.");
        ImGui::Separator();
        ImGui::BeginChild("ImportList", ImVec2(0, -55), ImGuiChildFlags_Borders);
        for (size_t Index = 0; Index < __ImportTasks.size(); ++Index)
        {
            auto& Task = __ImportTasks[Index];
            ImGui::PushID(static_cast<int>(Index));
            ImGui::TextWrapped("%s", Label(Task.__Filename).ToUtf8().c_str());
            ImGui::TextWrapped("Destination: %s", Task.__DestinationPath.ToUtf8().c_str());
            std::filesystem::path File;
            FString Package, Name;
            if (!Task.__ReplaceExisting && Packages() && Packages()->Resolve(Task.__DestinationPath, File, Package, Name))
            {
                char AssetName[512] = {};
                strncpy_s(AssetName, Name.ToUtf8().c_str(), _TRUNCATE);
                if (ImGui::InputText("Asset Name", AssetName, sizeof(AssetName)))
                {
                    const auto NewName = FAssetTools::SanitizeObjectName(FString(AssetName));
                    Packages()->GetObjectPath(File.parent_path() / (NewName + TEXT(".uasset")).ToWide(), Task.__DestinationPath);
                }
            }
            ImGui::Checkbox("Replace Existing", &Task.__ReplaceExisting);
            ImGui::Separator();
            ImGui::PopID();
        }
        ImGui::EndChild();
        if (ImGui::Button("Import", ImVec2(110, 0)))
        {
            std::vector<UAssetImportTask> One;
            One.push_back(std::move(__ImportTasks.front()));
            __ImportTasks.erase(__ImportTasks.begin());
            RunImportTasks(One);
            if (__ImportTasks.empty()) { __ShowImport = false; ImGui::CloseCurrentPopup(); }
        }
        ImGui::SameLine();
        if (ImGui::Button("Import All", ImVec2(110, 0)))
        {
            RunImportTasks(__ImportTasks); __ImportTasks.clear(); __ShowImport = false; ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(110, 0)))
        { __ImportTasks.clear(); __ShowImport = false; ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
    if (__ShowAsset && dynamic_cast<UMaterialInterface*>(__PreviewAsset)) DrawMaterialEditor();
    else if (__ShowAsset)
    {
        const bool Texture = dynamic_cast<UTexture2D*>(__PreviewAsset) != nullptr;
        const FString Title = Label(__PreviewPath.stem()) + (Texture ? " - Texture Editor###AssetEditor" : " - Sound Wave Editor###AssetEditor");
        ImGui::SetNextWindowSize(ImVec2(880, 620), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(Title.ToUtf8().c_str(), &__ShowAsset))
        {
            if (ImGui::Button("Reimport")) ReimportAsset(__PreviewPath, false);
            ImGui::SameLine();
            if (ImGui::Button("Reimport With New File...")) ReimportAsset(__PreviewPath, true);
            ImGui::Separator();
            ImGui::BeginChild("AssetViewport", ImVec2(ImGui::GetContentRegionAvail().x * 0.62f, 0), ImGuiChildFlags_Borders);
            if (const auto* Image = dynamic_cast<UTexture2D*>(__PreviewAsset))
            {
                if (__PreviewTexture)
                {
                    const ImVec2 Space = ImGui::GetContentRegionAvail();
                    const float Scale = (std::min)(Space.x / Image->GetSizeX(), Space.y / Image->GetSizeY());
                    if (Scale > 0) ImGui::Image(reinterpret_cast<ImTextureID>(__PreviewTexture.Get()), ImVec2(Image->GetSizeX()*Scale, Image->GetSizeY()*Scale));
                }
            }
            else if (const auto* Sound = dynamic_cast<USoundWave*>(__PreviewAsset))
            {
                if (ImGui::Button("Play") && __PreviewAudio) __PreviewAudio->Play();
                ImGui::SameLine();
                if (ImGui::Button("Stop") && __PreviewAudio) __PreviewAudio->Stop();
                ImGui::Text("Duration: %.2f seconds", Sound->GetDuration());
                ImGui::PlotHistogram("##Waveform", __PreviewWaveform.data(), static_cast<int>(__PreviewWaveform.size()), 0, nullptr, 0, 1, ImVec2(-1, 160));
            }
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild("AssetDetails", ImVec2(0, 0), ImGuiChildFlags_Borders);
            ImGui::TextUnformatted("Details");
            ImGui::Separator();
            if (__PreviewAsset) ImGui::TextWrapped("%s", __PreviewAsset->GetPathName().ToUtf8().c_str());
            if (const auto* Image = dynamic_cast<UTexture2D*>(__PreviewAsset))
            { ImGui::Text("Imported Size: %u x %u", Image->GetSizeX(), Image->GetSizeY()); ImGui::TextUnformatted("Format: RGBA8"); }
            if (const auto* Sound = dynamic_cast<USoundWave*>(__PreviewAsset))
            { ImGui::Text("Sample Rate: %u Hz", Sound->GetSampleRate()); ImGui::Text("Channels: %u", Sound->GetNumChannels()); }
            if (ImGui::CollapsingHeader("Import Settings", ImGuiTreeNodeFlags_DefaultOpen) && Packages() && __PreviewAsset)
                ImGui::TextWrapped("Source File: %s", Label(FReimportManager::Instance()->GetSourceFilename(*Packages(), __PreviewAsset->GetPathName())).ToUtf8().c_str());
            ImGui::EndChild();
        }
        ImGui::End();
        if (!__ShowAsset && __PreviewAudio) __PreviewAudio->Stop();
    }
    if (ImGui::GetTime() < __NotificationUntil && !__AssetMessage.IsEmpty())
    {
        const auto* Viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(Viewport->WorkPos.x + Viewport->WorkSize.x - 16,
            Viewport->WorkPos.y + Viewport->WorkSize.y - 40), ImGuiCond_Always, ImVec2(1,1));
        ImGui::SetNextWindowSize(ImVec2(460, 0));
        if (ImGui::Begin("Asset Import Results", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing))
        { ImGui::TextWrapped("%s", __AssetMessage.ToUtf8().c_str()); if (ImGui::SmallButton("Dismiss")) __NotificationUntil = 0; }
        ImGui::End();
    }
}

void FEditorImGui::DrawWorldOutliner()
{
    ImGui::Begin("World Outliner");
    static char Search[128] = {};
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##ActorSearch", "Search Actors", Search, sizeof(Search));
    if (auto* Engine = dynamic_cast<UGameEngine*>(GEngine))
        Engine->WithWorld([&](UWorld* _World)
        {
            const auto Actors = __EditorActorSubsystem ? __EditorActorSubsystem->GetAllLevelActors(_World) : std::vector<AActor*>();
            if (std::find(Actors.begin(), Actors.end(), __SelectedActor) == Actors.end()) __SelectedActor = nullptr;
            if (Actors.empty()) ImGui::TextDisabled("Use + Add to place actors, or drag assets into the Viewport.");
            size_t Index = 0;
            for (auto* Actor : Actors)
            {
                FString Name = Actor->GetName();
                if (Name.IsEmpty())
                {
                    Name = typeid(*Actor).name();
                    if (Name.StartsWith(TEXT("class "), ESearchCase::CaseSensitive)) Name.RemoveAt(0, 6);
                    if (Name.Len() > 1 && Name[0] == 'A') Name.RemoveAt(0, 1);
                    Name += "_" + std::to_string(Index);
                }
                ++Index;
                for (auto* Component : Actor->GetComponents())
                {
                    if (auto* Billboard = dynamic_cast<UBillboardComponent*>(Component); Billboard && Billboard->GetSprite())
                        Name = Billboard->GetSprite()->GetName();
                    if (auto* Audio = dynamic_cast<UAudioComponent*>(Component); Audio && Audio->GetSound())
                        Name = Audio->GetSound()->GetName();
                }
                if (_World && _World->GetAuthGameMode() && _World->GetAuthGameMode()->GetDefaultPawn() == Actor) Name = "Player";
                if (!Matches(Name, Search)) continue;
                ImGui::PushID(Actor);
                if (ImGui::Selectable(Name.ToUtf8().c_str(), __SelectedActor == Actor)) __SelectedActor = Actor;
                ImGui::PopID();
            }
            ImGui::Separator();
            ImGui::Text("%zu actors", Actors.size());
        });
    ImGui::End();
}

void FEditorImGui::SaveEditorAssetActors()
{
    if (!SaveLevelFile()) return;
    FString Error;
    if (!Packages()->SaveDirtyPackages(Error)) { __AssetMessage = Error; __NotificationUntil = ImGui::GetTime() + 8; }
}

void FEditorImGui::DrawDetails()
{
    ImGui::Begin("Details");
    if (auto* Engine = dynamic_cast<UGameEngine*>(GEngine))
        Engine->WithWorld([&](UWorld* _World)
        {
            const auto Actors = __EditorActorSubsystem ? __EditorActorSubsystem->GetAllLevelActors(_World) : std::vector<AActor*>();
            if (std::find(Actors.begin(), Actors.end(), __SelectedActor) == Actors.end()) __SelectedActor = nullptr;
            if (__DetailsActor != __SelectedActor) { __DetailsActor = __SelectedActor; __SelectedComponent = nullptr; }
            if (!__SelectedActor) { ImGui::TextDisabled("Select an actor to view details."); return; }
            auto* Actor = __SelectedActor;
            if (__SelectedComponent && !Actor->OwnsComponent(__SelectedComponent)) __SelectedComponent = nullptr;
            auto TypeName = [](const UObject* _Object)
            {
                FString Name = typeid(*_Object).name();
                if (Name.StartsWith(TEXT("class "), ESearchCase::CaseSensitive)) Name.RemoveAt(0, 6);
                return Name;
            };
            auto ObjectLabel = [&](const UObject* _Object)
            {
                const FString Name = _Object->GetName();
                return Name.IsEmpty() ? TypeName(_Object) : Name;
            };
            ImGui::TextUnformatted(ObjectLabel(Actor).ToUtf8().c_str());
            if (ImGui::Button("+ Add")) ImGui::OpenPopup("AddComponent");
            if (ImGui::BeginPopup("AddComponent"))
            {
                USceneComponent* Added = nullptr;
                if (ImGui::MenuItem("Scene")) Added = Actor->CreateInstanceComponent<USceneComponent>();
                if (ImGui::MenuItem("Box Collision")) Added = Actor->CreateInstanceComponent<UBoxComponent>();
                if (ImGui::MenuItem("Sphere Collision")) Added = Actor->CreateInstanceComponent<USphereComponent>();
                if (ImGui::MenuItem("Capsule Collision")) Added = Actor->CreateInstanceComponent<UCapsuleComponent>();
                if (ImGui::MenuItem("Billboard")) Added = Actor->CreateInstanceComponent<UBillboardComponent>();
                if (ImGui::MenuItem("Audio")) Added = Actor->CreateInstanceComponent<UAudioComponent>();
                if (Added)
                {
                    if (Actor->GetRootComponent()) Added->SetupAttachment(Actor->GetRootComponent());
                    else Actor->SetRootComponent(Added);
                    __SelectedComponent = Added;
                }
                ImGui::EndPopup();
            }
            ImGui::BeginChild("Components", ImVec2(0, 180), ImGuiChildFlags_Borders);
            if (ImGui::Selectable((ObjectLabel(Actor) + TEXT(" (Self)")).ToUtf8().c_str(), !__SelectedComponent)) __SelectedComponent = nullptr;
            std::vector<UActorComponent*> Components(Actor->GetComponents().begin(), Actor->GetComponents().end());
            std::sort(Components.begin(), Components.end(), [&](auto* _A, auto* _B)
            {
                if (_A == Actor->GetRootComponent() || _B == Actor->GetRootComponent())
                    return _A == Actor->GetRootComponent() && _B != Actor->GetRootComponent();
                const auto A = ObjectLabel(_A), B = ObjectLabel(_B);
                return A == B ? std::less<UActorComponent*>()(_A, _B) : A < B;
            });
            std::set<UActorComponent*> Visited;
            std::function<void(UActorComponent*)> DrawComponent = [&](UActorComponent* _Component)
            {
                if (!Actor->OwnsComponent(_Component) || !Visited.insert(_Component).second) return;
                auto* Scene = dynamic_cast<USceneComponent*>(_Component);
                const bool Children = Scene && !Scene->GetAttachChildren().empty();
                const FString Label = ObjectLabel(_Component) + (_Component == Actor->GetRootComponent() ? " (Root)" : "");
                ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;
                if (!Children) Flags |= ImGuiTreeNodeFlags_Leaf;
                if (__SelectedComponent == _Component) Flags |= ImGuiTreeNodeFlags_Selected;
                const bool Open = ImGui::TreeNodeEx(_Component, Flags, "%s", Label.ToUtf8().c_str());
                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) __SelectedComponent = _Component;
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", TypeName(_Component).ToUtf8().c_str());
                if (Open)
                {
                    if (Scene) for (auto* Child : Scene->GetAttachChildren()) DrawComponent(Child);
                    ImGui::TreePop();
                }
            };
            for (auto* Component : Components)
            {
                auto* Scene = dynamic_cast<USceneComponent*>(Component);
                if (!Scene || !Scene->GetAttachParent() || !Actor->OwnsComponent(Scene->GetAttachParent())) DrawComponent(Component);
            }
            ImGui::EndChild();
            ImGui::SetNextItemWidth(-1);
            ImGui::InputTextWithHint("##DetailsSearch", "Search Details", __DetailsSearch, sizeof(__DetailsSearch));
            auto Section = [&](const char* _Name, const char* _Properties)
            {
                return (Matches(_Name, __DetailsSearch) || Matches(_Properties, __DetailsSearch)) && ImGui::CollapsingHeader(_Name, ImGuiTreeNodeFlags_DefaultOpen);
            };
            UObject* Selected = __SelectedComponent ? static_cast<UObject*>(__SelectedComponent) : Actor;
            ImGui::PushID(Selected);
            auto* Scene = __SelectedComponent ? dynamic_cast<USceneComponent*>(__SelectedComponent) : Actor->GetRootComponent();
            if (Scene && Section("Transform", "Location X Y Z World Relative"))
            {
                const bool Relative = __SelectedComponent && Scene != Actor->GetRootComponent();
                ImGui::TextDisabled(Relative ? "Relative to parent" : "World space");
                const FVector Position = Relative ? Scene->GetRelativeLocation() : Scene->GetWorldLocation();
                float Values[3] = {float(Position.X), float(Position.Y), float(Position.Z)};
                bool Edited = false;
                if (ImGui::BeginTable("LocationRow", 4, ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 65);
                    ImGui::TableSetupColumn("X");
                    ImGui::TableSetupColumn("Y");
                    ImGui::TableSetupColumn("Z");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Location");
                    const ImVec4 Colors[] = {ImVec4(0.42f, 0.12f, 0.12f, 1), ImVec4(0.12f, 0.32f, 0.12f, 1), ImVec4(0.12f, 0.2f, 0.42f, 1)};
                    const char* Formats[] = {"X: %.3f", "Y: %.3f", "Z: %.3f"};
                    for (int Axis = 0; Axis < 3; ++Axis)
                    {
                        ImGui::TableNextColumn();
                        ImGui::PushID(Axis);
                        ImGui::PushStyleColor(ImGuiCol_FrameBg, Colors[Axis]);
                        ImGui::SetNextItemWidth(-1);
                        Edited |= ImGui::DragFloat("##Value", &Values[Axis], 1.0f, 0, 0, Formats[Axis]);
                        ImGui::PopStyleColor();
                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }
                if (Edited && std::isfinite(Values[0]) && std::isfinite(Values[1]) && std::isfinite(Values[2]))
                {
                    if (Relative) Scene->SetRelativeLocation(FVector(Values[0], Values[1], Values[2]));
                    else Scene->SetWorldLocation(FVector(Values[0], Values[1], Values[2]));
                }
                if (ImGui::SmallButton("Reset Location"))
                {
                    if (Relative) Scene->SetRelativeLocation(FVector(0.0, 0.0, 0.0));
                    else Scene->SetWorldLocation(FVector(0.0, 0.0, 0.0));
                }
                const auto Rotation = Scene->GetRelativeRotationQuaternion().Rotator();
                float Angles[] = {float(Rotation.Roll), float(Rotation.Pitch), float(Rotation.Yaw)};
                if (ImGui::DragFloat3("Rotation", Angles, .5f))
                    Scene->SetRelativeRotation(FRotator(Angles[1], Angles[2], Angles[0]).Quaternion());
                const auto Scale = Scene->GetRelativeScale3D();
                float Scales[] = {float(Scale.X), float(Scale.Y), float(Scale.Z)};
                if (ImGui::DragFloat3("Scale", Scales, .01f, .001f, 10000.f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                    Scene->SetRelativeScale3D(FVector(Scales[0], Scales[1], Scales[2]));
            }
            auto DrawProperties = [&](UActorComponent* _Component)
            {
                ImGui::PushID(_Component);
                if (auto* Primitive = dynamic_cast<UPrimitiveComponent*>(_Component))
                    DrawCollisionDetails(Primitive);
                if (auto* Mesh = dynamic_cast<UMeshComponent*>(_Component))
                    if (Section("Materials", "Material Element Slot"))
                    {
                        UObject* Asset = Mesh->GetMaterial(0);
                        if (DrawAssetProperty("Element 0", Asset, 4)) Mesh->SetMaterial(0, static_cast<UMaterialInterface*>(Asset));
                    }
                if (auto* Billboard = dynamic_cast<UBillboardComponent*>(_Component))
                    if (Section("Sprite", "Billboard Texture"))
                    {
                        UObject* Asset = Billboard->GetSprite();
                        if (DrawAssetProperty("Sprite", Asset, true)) Billboard->SetSprite(static_cast<UTexture2D*>(Asset));
                    }
                if (auto* Audio = dynamic_cast<UAudioComponent*>(_Component))
                    if (Section("Sound", "Audio Volume Multiplier Pitch Looping Play Stop"))
                    {
                        UObject* Asset = Audio->GetSound();
                        if (DrawAssetProperty("Sound", Asset, false)) Audio->SetSound(static_cast<USoundWave*>(Asset));
                        float Volume = Audio->GetVolumeMultiplier(), Pitch = Audio->GetPitchMultiplier();
                        bool Looping = Audio->IsLooping();
                        if (ImGui::SliderFloat("Volume Multiplier", &Volume, 0.0f, 2.0f)) Audio->SetVolumeMultiplier(Volume);
                        if (ImGui::SliderFloat("Pitch Multiplier", &Pitch, 0.1f, 4.0f)) Audio->SetPitchMultiplier(Pitch);
                        if (ImGui::Checkbox("Looping", &Looping))
                        {
                            const bool Playing = Audio->IsPlaying(); Audio->SetLooping(Looping); if (Playing) Audio->Play();
                        }
                        if (ImGui::Button("Play")) Audio->Play();
                        ImGui::SameLine();
                        if (ImGui::Button("Stop")) Audio->Stop();
                    }
                if (auto* MeshComponent = dynamic_cast<UStaticMeshComponent*>(_Component))
                    if (Section("Static Mesh", "Mesh Width Height Color"))
                    {
                        auto* Mesh = MeshComponent->GetStaticMesh();
                        ImGui::Text("Static Mesh: %s", Mesh ? ObjectLabel(Mesh).ToUtf8().c_str() : "None");
                        if (Mesh)
                        {
                            int Size[] = {Mesh->GetWidth(), Mesh->GetHeight()};
                            const auto Color = Mesh->GetColor();
                            float Channels[] = {Color.R / 255.f, Color.G / 255.f, Color.B / 255.f, Color.A / 255.f};
                            bool Changed = ImGui::DragInt2("Size", Size, 1, 1, 65536, "%d", ImGuiSliderFlags_AlwaysClamp);
                            Changed |= ImGui::ColorEdit4("Color", Channels);
                            if (Changed) MeshComponent->RestoreMesh(Size[0], Size[1], FColor(uint8(Channels[0] * 255), uint8(Channels[1] * 255), uint8(Channels[2] * 255), uint8(Channels[3] * 255)));
                        }
                    }
                if (auto* Input = dynamic_cast<UInputComponent*>(_Component))
                    if (Section("Input", "Priority Block Input"))
                    {
                        ImGui::InputInt("Priority", &Input->__Priority);
                        ImGui::Checkbox("Block Input", &Input->__bBlockInput);
                    }
                ImGui::PopID();
            };
            if (__SelectedComponent) DrawProperties(__SelectedComponent);
            else for (auto* Component : Components)
            {
                ImGui::PushID(Component);
                if (Components.size() > 1) ImGui::SeparatorText(ObjectLabel(Component).ToUtf8().c_str());
                DrawProperties(Component);
                ImGui::PopID();
            }
            if (Section(__SelectedComponent ? "Component Tick" : "Actor Tick", "Can Ever Tick Tick Group"))
            {
                const FTickFunction& Tick = __SelectedComponent ? static_cast<const FTickFunction&>(__SelectedComponent->PrimaryComponentTick) : Actor->PrimaryActorTick;
                ImGui::Text("Can Ever Tick: %s", Tick.bCanEverTick ? "True" : "False");
                const char* Groups[] = {"Pre Physics", "Start Physics", "During Physics", "End Physics", "Post Physics", "Post Update Work"};
                const int Group = static_cast<int>(Tick.TickGroup);
                ImGui::Text("Tick Group: %s", Group >= 0 && Group < 6 ? Groups[Group] : "Unknown");
            }
            if (Section("Object", "Class Name Owner Registered"))
            {
                ImGui::Text("Class: %s", TypeName(Selected).ToUtf8().c_str());
                ImGui::Text("Name: %s", ObjectLabel(Selected).ToUtf8().c_str());
                if (__SelectedComponent)
                {
                    ImGui::Text("Owner: %s", ObjectLabel(Actor).ToUtf8().c_str());
                    ImGui::Text("Registered: %s", __SelectedComponent->GetWorld() ? "True" : "False");
                }
            }
            ImGui::PopID();
        });
    ImGui::End();
}
