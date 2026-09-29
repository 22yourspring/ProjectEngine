#include "framework.h"
#include "EditorImGui.h"
#include "EditorActorSubsystem.h"
#include "UE/GameEngine.h"
#include "UE/PathEngineSystem.h"
#include "UE/ResourceEngineSystem.h"
#include "UE/WorldPersistence.h"
#include "UE/World.h"
#include "UE/SceneComponent.h"
#include "UE/PlayerStart.h"
#include <cmath>
#include <commdlg.h>
#include <fstream>
#include "ThirdParty/WIL/include/wil/resource.h"

namespace
{
    PathEngineSystem* Paths() { return GEngine ? GEngine->GetEngineSystem<PathEngineSystem>() : nullptr; }
    UGameEngine* Engine() { return dynamic_cast<UGameEngine*>(GEngine); }
    std::filesystem::path SettingsFile() { return Paths()->GetProjectSavedDirectory() / L"Config/EditorPerProjectUserSettings.ini"; }
    std::filesystem::path RecoveryFile() { return Paths()->GetProjectSavedDirectory() / L"Autosaves/EditorRecovery.umap"; }
    FString ReadSetting(const wchar_t* _Key)
    {
        wchar_t Text[32768] = {}; GetPrivateProfileStringW(L"Editor", _Key, L"", Text, 32768, SettingsFile().c_str()); return Text;
    }
    bool WriteSetting(const wchar_t* _Key, const FString& _Value)
    {
        std::error_code Error; std::filesystem::create_directories(SettingsFile().parent_path(), Error);
        return !Error && WritePrivateProfileStringW(L"Editor", _Key, _Value.c_str(), SettingsFile().c_str());
    }
}
bool FEditorImGui::UseEngineGameMode() const
{
    return __LevelGameMode < 0 ? __MapsSettings.__UseEngineGameMode : __LevelGameMode == 0;
}
void FEditorImGui::RegisterProjectFileAction()
{
    wchar_t Executable[32768] = {}; GetModuleFileNameW(nullptr, Executable, 32768);
    const FString Command = L"\"" + FString(Executable) + L"\" \"%1\"";
    auto Write = [](const wchar_t* _Key, const wchar_t* _Name, const FString& _Value)
    {
        wil::unique_hkey Key;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, _Key, 0, nullptr, 0, KEY_SET_VALUE, nullptr, Key.put(), nullptr) != ERROR_SUCCESS) return false;
        return RegSetValueExW(Key.get(), _Name, 0, REG_SZ, reinterpret_cast<const BYTE*>(_Value.c_str()), static_cast<DWORD>((_Value.Len() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
    };
    const bool Saved = Write(L"Software\\Classes\\SystemFileAssociations\\.uproject\\shell\\UnrealEngine", nullptr, L"Open with UnrealEngine") &&
        Write(L"Software\\Classes\\SystemFileAssociations\\.uproject\\shell\\UnrealEngine\\command", nullptr, Command) &&
        Write(L"Software\\Classes\\UnrealEngine.ProjectFile", nullptr, L"UnrealEngine Project") &&
        Write(L"Software\\Classes\\UnrealEngine.ProjectFile\\shell\\open\\command", nullptr, Command) &&
        Write(L"Software\\Classes\\.uproject\\OpenWithProgids", L"UnrealEngine.ProjectFile", L"");
    __AssetMessage = Saved ? "Project files now offer Open with UnrealEngine in Explorer. Use Windows Open with to choose the default app." : "Could not register the project file action.";
    __NotificationUntil = ImGui::GetTime() + 8;
}
void FEditorImGui::InitializeProjectSession()
{
    if (!Paths() || !Engine()) return;
    __MapsSettings.Load(Paths()->GetProjectDirectory());
    __LoadLastLevel = ReadSetting(L"LoadLastLevel") == L"1";
    __AutoSaveLevels = ReadSetting(L"AutoSaveLevels") != L"0";
    __AutoSaveMinutes = (std::clamp)(static_cast<int>(GetPrivateProfileIntW(L"Editor", L"AutoSaveMinutes", 5, SettingsFile().c_str())), 1, 60);
    __ProjectSessionReady = true;
    Engine()->LoadEditorMap(L"Untitled");
    __NextAutoSave = ImGui::GetTime() + __AutoSaveMinutes * 60;
    auto Startup = UGameMapsSettings::ResolveMap(Paths()->GetProjectContentDirectory(), __MapsSettings.__EditorStartupMap);
    if (__LoadLastLevel && !ReadSetting(L"LastLevel").IsEmpty()) Startup = ReadSetting(L"LastLevel").ToWide();
    if (!Startup.empty()) OpenLevelFile(Startup);
    else
    {
        Engine()->LoadEditorMap(L"Untitled");
        __EditorActorSubsystem = std::make_unique<UEditorActorSubsystem>();
        __EditorActorSubsystem->LoadEditorActors(Paths()->GetProjectSavedDirectory() / L"Config/EditorAssetActors.txt");
        auto& Packages = GEngine->GetEngineSystem<ResourceEngineSystem>()->GetPackageStore();
        Engine()->WithWorld([&](UWorld* _World) { __EditorActorSubsystem->RestoreEditorActors(_World, Packages); });
        __SavedLevelData.clear(); RefreshLevelDirty();
        if (!__LevelDirty) Engine()->WithWorld([&](UWorld* _World) { FWorldPersistence::Capture(_World, __SavedLevelData, __AssetMessage); });
    }
    __RecoveryOriginal = ReadSetting(L"RecoveryOriginal").ToWide();
    __RecoveryPrompt = ReadSetting(L"RecoveryAvailable") == L"1" && std::filesystem::is_regular_file(RecoveryFile());
}
std::filesystem::path FEditorImGui::ChooseFile(bool _Project, bool _Save)
{
    wchar_t File[32768] = {};
    const auto Initial = _Project ? Paths()->GetRootDirectory() : Paths()->GetProjectContentDirectory();
    OPENFILENAMEW Dialog = {}; Dialog.lStructSize = sizeof(Dialog); Dialog.hwndOwner = __MainWindow;
    Dialog.lpstrFile = File; Dialog.nMaxFile = 32768; Dialog.lpstrInitialDir = Initial.c_str();
    Dialog.lpstrFilter = _Project ? L"Project Files (*.uproject)\0*.uproject\0\0" : L"Level Packages (*.umap)\0*.umap\0\0";
    Dialog.lpstrDefExt = _Project ? L"uproject" : L"umap";
    Dialog.Flags = OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | (_Save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    return (_Save ? GetSaveFileNameW(&Dialog) : GetOpenFileNameW(&Dialog)) ? std::filesystem::path(File) : std::filesystem::path();
}
void FEditorImGui::RefreshLevelDirty()
{
    if (!__ProjectSessionReady || __IsPlaying || __WaitingForWorldChange) return;
    std::vector<uint8> Current; FString Error;
    bool Captured = false;
    Engine()->WithWorld([&](UWorld* _World) { Captured = FWorldPersistence::Capture(_World, Current, Error); });
    __LevelDirty = !Captured || Current != __SavedLevelData || __LevelGameMode != __SavedLevelGameMode;
    if (__EditedMap.empty() && __SavedLevelData.empty() && Captured && Current.size() == 4 && __LevelGameMode == -1) { __SavedLevelData = Current; __LevelDirty = false; }
    const auto Title = L"UnrealEngine Editor - " + Paths()->GetProjectFile().stem().wstring() + L" - " +
        (__EditedMap.empty() ? FString(L"Untitled") : __EditedMap.stem().wstring()) + (__LevelDirty ? L" *" : L"");
    SetWindowTextW(__MainWindow, Title.c_str());
}
bool FEditorImGui::SaveLevelFile(bool _SaveAs)
{
    if (__IsPlaying || __WaitingForWorldChange) { __AssetMessage = "Stop Play before saving a level."; __NotificationUntil = ImGui::GetTime() + 5; return false; }
    auto File = __EditedMap;
    if (_SaveAs || File.empty() || UGameMapsSettings::MapName(Paths()->GetProjectContentDirectory(), File).IsEmpty()) File = ChooseFile(false, true);
    if (File.empty()) return false;
    if (_wcsicmp(File.extension().c_str(), L".umap") || UGameMapsSettings::MapName(Paths()->GetProjectContentDirectory(), File).IsEmpty())
    { __AssetMessage = "Save levels as .umap files inside this project's Content folder."; __NotificationUntil = ImGui::GetTime() + 6; return false; }
    std::vector<uint8> Data; bool Captured = false;
    Engine()->WithWorld([&](UWorld* _World) { Captured = FWorldPersistence::Capture(_World, Data, __AssetMessage); });
    if (!Captured || !FWorldPersistence::Write(File, Data, __LevelGameMode, __AssetMessage)) { __NotificationUntil = ImGui::GetTime() + 8; return false; }
    __EditedMap = File; __SavedLevelData = std::move(Data); __SavedLevelGameMode = __LevelGameMode; __LevelDirty = false;
    Engine()->RenameEditorMap(File.stem().c_str());
    WriteSetting(L"LastLevel", File.wstring()); WriteSetting(L"RecoveryAvailable", L"0");
    __AssetMessage = "Saved level: " + FString(File.filename().wstring()).ToUtf8(); __NotificationUntil = ImGui::GetTime() + 5;
    __NextAssetRefresh = 0; RefreshLevelDirty(); return true;
}
bool FEditorImGui::OpenLevelFile(const std::filesystem::path& _Path)
{
    if (!IsEditorMap(_Path)) { __AssetMessage = "Unsupported or corrupt level package."; __NotificationUntil = ImGui::GetTime() + 8; return false; }
    std::vector<uint8> Data; int Mode = -1; FString Error;
    auto& Packages = GEngine->GetEngineSystem<ResourceEngineSystem>()->GetPackageStore();
    std::unique_ptr<UWorld> World;
    bool Legacy = !FWorldPersistence::Read(_Path, Data, Mode, Error);
    if (!Legacy) World = FWorldPersistence::Restore(Data, FString(_Path.stem().wstring()), Packages, Error);
    else
    {
        World = FWorldPersistence::CreateWorld(FString(_Path.stem().wstring()));
        std::ifstream LegacyMap(_Path);
        std::string Token;
        while (LegacyMap >> Token)
        {
            if (Token == "PlayerLocation")
            {
                FVector Position;
                if (!(LegacyMap >> Position.X >> Position.Y >> Position.Z) || !std::isfinite(Position.X) || !std::isfinite(Position.Y) || !std::isfinite(Position.Z))
                { __AssetMessage = "Invalid legacy player start."; __NotificationUntil = ImGui::GetTime() + 6; return false; }
                World->SpawnActor<APlayerStart>()->SetActorLocation(Position);
            }
            else { std::string Ignored; std::getline(LegacyMap, Ignored); }
        }
        UEditorActorSubsystem LegacyActors;
        const auto Previous = __EditedMap; __EditedMap = _Path;
        LegacyActors.LoadEditorActors(GetEditorActorsSavePath()); __EditedMap = Previous;
        LegacyActors.RestoreEditorActors(World.get(), Packages);
        FWorldPersistence::Capture(World.get(), Data, Error);
    }
    if (!World) { __AssetMessage = Error; __NotificationUntil = ImGui::GetTime() + 8; return false; }
    Engine()->SetEditorWorld(std::move(World), _Path.stem().c_str());
    __EditedMap = _Path; __SavedLevelData = std::move(Data); __LevelGameMode = __SavedLevelGameMode = Mode;
    __EditorWorldGeneration = Engine()->GetWorldGeneration(); __WaitingForWorldChange = false;
    __SelectedActor = nullptr; __SelectedComponent = nullptr; __DetailsActor = nullptr;
    __EditorActorSubsystem = std::make_unique<UEditorActorSubsystem>(); __LevelActorSubsystems.clear();
    __LevelDirty = Legacy; if (Legacy) __SavedLevelData.clear();
    WriteSetting(L"LastLevel", _Path.wstring());
    __AssetMessage = Legacy ? "Opened legacy level. Save to migrate its placement data into the level package." : "Opened level: " + FString(_Path.filename().wstring()).ToUtf8();
    __NotificationUntil = ImGui::GetTime() + 6; RefreshLevelDirty(); return true;
}
void FEditorImGui::RequestClose() { RequestFileAction(EFileAction::Exit); }
void FEditorImGui::RequestFileAction(EFileAction _Action, const std::filesystem::path& _Path)
{
    if (!__ProjectSessionReady) { if (_Action == EFileAction::Exit) DestroyWindow(__MainWindow); return; }
    if (__IsPlaying || __WaitingForWorldChange) { __AssetMessage = "Stop Play before closing or changing the project or level."; __NotificationUntil = ImGui::GetTime() + 6; return; }
    if (__FileAction != EFileAction::None) return;
    auto File = _Path;
    if ((_Action == EFileAction::OpenProject || _Action == EFileAction::OpenLevel) && File.empty()) File = ChooseFile(_Action == EFileAction::OpenProject, false);
    if ((_Action == EFileAction::OpenProject || _Action == EFileAction::OpenLevel) && File.empty()) return;
    RefreshLevelDirty(); __FileAction = _Action; __ActionPath = File;
    if (__LevelDirty || GEngine->GetEngineSystem<ResourceEngineSystem>()->GetPackageStore().HasDirtyPackages()) __ConfirmFileAction = true;
    else ExecuteFileAction();
}
void FEditorImGui::ExecuteFileAction()
{
    const auto Action = __FileAction; __FileAction = EFileAction::None; __ConfirmFileAction = false;
    if (Action == EFileAction::OpenLevel) OpenLevelFile(__ActionPath);
    else if (Action == EFileAction::NewLevel)
    {
        Engine()->LoadEditorMap(L"Untitled"); __EditedMap.clear(); __SavedLevelData.clear();
        __LevelGameMode = __SavedLevelGameMode = -1; __WaitingForWorldChange = false;
        __SelectedActor = nullptr; __DetailsActor = nullptr; __SelectedComponent = nullptr;
        __EditorActorSubsystem = std::make_unique<UEditorActorSubsystem>();
        WriteSetting(L"LastLevel", L""); RefreshLevelDirty();
    }
    else if (Action == EFileAction::OpenProject)
    {
        PathEngineSystem Candidate;
        if (FAILED(Candidate.ConfigureFromProjectFile(__ActionPath)) || !std::filesystem::is_regular_file(Candidate.GetProjectModuleFile(L"Editor")))
        { __AssetMessage = "Project descriptor or built Editor game module is missing. Build the project first."; __NotificationUntil = ImGui::GetTime() + 8; return; }
        wchar_t Executable[32768] = {}; GetModuleFileNameW(nullptr, Executable, 32768);
        FString Command = L"\"" + FString(Executable) + L"\" \"" + __ActionPath.wstring() + L"\"";
        STARTUPINFOW Startup = {}; Startup.cb = sizeof(Startup); wil::unique_process_information Process;
        if (!CreateProcessW(Executable, &Command[0], nullptr, nullptr, FALSE, 0, nullptr, Candidate.GetProjectDirectory().c_str(), &Startup, Process.addressof()))
        { __AssetMessage = "Could not start the selected project."; __NotificationUntil = ImGui::GetTime() + 8; return; }
        DestroyWindow(__MainWindow);
    }
    else if (Action == EFileAction::Exit) DestroyWindow(__MainWindow);
}
void FEditorImGui::SaveSessionSettings()
{
    if (!WriteSetting(L"LoadLastLevel", __LoadLastLevel ? L"1" : L"0") ||
        !WriteSetting(L"AutoSaveLevels", __AutoSaveLevels ? L"1" : L"0") ||
        !WriteSetting(L"AutoSaveMinutes", std::to_wstring(__AutoSaveMinutes)))
    { __AssetMessage = "Could not save Editor preferences."; __NotificationUntil = ImGui::GetTime() + 6; }
    __NextAutoSave = ImGui::GetTime() + __AutoSaveMinutes * 60;
}
void FEditorImGui::TickProjectSession()
{
    if (!__ProjectSessionReady || __IsPlaying || __WaitingForWorldChange) return;
    if (ImGui::GetTime() >= __NextDirtyCheck) { RefreshLevelDirty(); __NextDirtyCheck = ImGui::GetTime() + 0.3; }
    if (__AutoSaveLevels && ImGui::GetTime() >= __NextAutoSave && !__ConfirmFileAction && !__RecoveryPrompt)
    {
        __NextAutoSave = ImGui::GetTime() + __AutoSaveMinutes * 60;
        if (!__LevelDirty) return;
        std::vector<uint8> Data; bool Captured = false;
        Engine()->WithWorld([&](UWorld* _World) { Captured = FWorldPersistence::Capture(_World, Data, __AssetMessage); });
        if (Captured && FWorldPersistence::Write(RecoveryFile(), Data, __LevelGameMode, __AssetMessage))
        {
            WriteSetting(L"RecoveryOriginal", __EditedMap.wstring()); WriteSetting(L"RecoveryAvailable", L"1");
            __AssetMessage = "Autosaved a recovery copy.";
        }
        __NotificationUntil = ImGui::GetTime() + 5;
    }
}
void FEditorImGui::DrawFileDialogs()
{
    if (!__ProjectSessionReady) return;
    const bool ModalOpen = __ConfirmFileAction || __RecoveryPrompt || ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId);
    if (!ModalOpen && !__IsPlaying && !ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Delete, false) && __SelectedActor)
    {
        Engine()->WithWorld([&](UWorld* _World) { if (_World) _World->DestroyActor(__SelectedActor); });
        __SelectedActor = nullptr; __DetailsActor = nullptr; __SelectedComponent = nullptr;
    }
    if (!ModalOpen && ImGui::GetIO().KeyCtrl && !ImGui::GetIO().WantTextInput)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_S, false)) { if (ImGui::GetIO().KeyShift) SaveEditorAssetActors(); else SaveLevelFile(ImGui::GetIO().KeyAlt); }
        if (ImGui::IsKeyPressed(ImGuiKey_O, false)) RequestFileAction(EFileAction::OpenLevel);
        if (ImGui::IsKeyPressed(ImGuiKey_N, false)) RequestFileAction(EFileAction::NewLevel);
    }
    if (__ConfirmFileAction) ImGui::OpenPopup("Save Content");
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Save Content", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("The level or assets have unsaved changes.");
        ImGui::TextUnformatted("Save before continuing?");
        if (ImGui::Button("Save", ImVec2(100, 0)))
        {
            FString Error;
            if (!GEngine->GetEngineSystem<ResourceEngineSystem>()->GetPackageStore().SaveDirtyPackages(Error)) __AssetMessage = Error;
            else if (!__LevelDirty || SaveLevelFile()) { ImGui::CloseCurrentPopup(); ExecuteFileAction(); }
        }
        ImGui::SameLine();
        if (ImGui::Button("Don't Save", ImVec2(100, 0))) { WriteSetting(L"RecoveryAvailable", L"0"); ImGui::CloseCurrentPopup(); ExecuteFileAction(); }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) { __FileAction = EFileAction::None; __ConfirmFileAction = false; ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
    if (__RecoveryPrompt) ImGui::OpenPopup("Restore Autosave");
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Restore Autosave", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("An unsaved level recovery copy is available.");
        if (ImGui::Button("Restore"))
        {
            if (OpenLevelFile(RecoveryFile()))
            {
                __EditedMap = __RecoveryOriginal; __SavedLevelData.clear(); __LevelDirty = true;
                Engine()->RenameEditorMap(__EditedMap.empty() ? L"Untitled" : __EditedMap.stem().c_str());
                WriteSetting(L"LastLevel", __EditedMap.wstring());
            }
            __RecoveryPrompt = false; ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Discard Recovery")) { WriteSetting(L"RecoveryAvailable", L"0"); __RecoveryPrompt = false; ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
}
void FEditorImGui::DrawMapSettings()
{
    const char* Modes[] = {"GameModeBase", "ProjectGameMode"};
    int Mode = __MapsSettings.__UseEngineGameMode ? 0 : 1;
    bool Changed = ImGui::Combo("Default GameMode", &Mode, Modes, 2);
    __MapsSettings.__UseEngineGameMode = Mode == 0;
    auto Map = [&](const char* _Label, FString& _Value)
    {
        if (ImGui::BeginCombo(_Label, _Value.IsEmpty() ? "None" : _Value.ToUtf8().c_str()))
        {
            if (ImGui::Selectable("None", _Value.IsEmpty())) { _Value = FString(); Changed = true; }
            for (const auto& File : __AssetFiles)
            {
                auto Name = UGameMapsSettings::MapName(Paths()->GetProjectContentDirectory(), File);
                if (File.extension() != L".umap" || Name.IsEmpty()) continue;
                if (ImGui::Selectable(Name.ToUtf8().c_str(), Name == _Value)) { _Value = Name; Changed = true; }
            }
            ImGui::EndCombo();
        }
    };
    Map("Editor Startup Map", __MapsSettings.__EditorStartupMap); Map("Game Default Map", __MapsSettings.__GameDefaultMap);
    if (Changed && !__MapsSettings.Save(Paths()->GetProjectDirectory())) { __AssetMessage = "Could not save project settings."; __NotificationUntil = ImGui::GetTime() + 6; }
}
void FEditorImGui::DrawAddActor()
{
    ImGui::BeginDisabled(__IsPlaying || __WaitingForWorldChange || !__ProjectSessionReady);
    if (ImGui::Button("+ Add")) ImGui::OpenPopup("Add Actor");
    if (ImGui::BeginPopup("Add Actor"))
    {
        static ImGuiTextFilter Search;
        if (ImGui::IsWindowAppearing()) { Search.Clear(); ImGui::SetKeyboardFocusHere(); }
        Search.Draw("Search Classes", 220);
        ImGui::SeparatorText("PLACE ACTORS");
        const auto Classes = FWorldPersistence::GetActorClasses();
        auto Entry = [&](const FString& _Class)
        {
            auto Name = _Class.Mid(_Class.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromEnd) + 1);
            if (Name == "Actor") Name = "Empty Actor";
            else if (Name == "Character") Name = "Empty Character";
            else if (Name == "Pawn") Name = "Empty Pawn";
            else if (Name == "PlayerStart") Name = "Player Start";
            else if (Name == "MannequinPawn") Name = "Mannequin Pawn";
            if (!Search.PassFilter(Name.ToUtf8().c_str()) && !Search.PassFilter(_Class.ToUtf8().c_str())) return;
            if (ImGui::MenuItem((Name + TEXT("##") + _Class).ToUtf8().c_str())) Engine()->WithWorld([&](UWorld* _World)
            {
                if (!_World) return;
                __SelectedActor = FWorldPersistence::SpawnActor(_Class, _World);
                if (__SelectedActor && !__SelectedActor->GetRootComponent()) __SelectedActor->SetRootComponent(__SelectedActor->CreateInstanceComponent<USceneComponent>());
                __SelectedComponent = nullptr;
                __DetailsActor = nullptr;
                if (!__SelectedActor) { __AssetMessage = "Could not place the selected actor."; __NotificationUntil = ImGui::GetTime() + 5; }
            });
        };
        if (Search.IsActive()) { for (const auto& Class : Classes) Entry(Class); }
        else
        {
            if (ImGui::BeginMenu("Basic"))
            {
                for (const auto& Class : Classes) if (Class.StartsWith(TEXT("/Script/Engine."), ESearchCase::CaseSensitive)) Entry(Class);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Project"))
            {
                for (const auto& Class : Classes) if (!Class.StartsWith(TEXT("/Script/Engine."), ESearchCase::CaseSensitive)) Entry(Class);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("All Classes"))
            {
                for (const auto& Class : Classes) Entry(Class);
                ImGui::EndMenu();
            }
        }
        ImGui::EndPopup();
    }
    ImGui::EndDisabled();
}
