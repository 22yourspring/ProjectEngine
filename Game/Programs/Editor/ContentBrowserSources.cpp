#include "framework.h"
#include "EditorImGui.h"
#include "UE/Engine.h"
#include "UE/GameEngine.h"
#include "UE/WorldPersistence.h"
#include "UE/World.h"
#include "UE/SceneComponent.h"
#include "UE/PathEngineSystem.h"
#include <algorithm>
#include <set>
#include <cctype>

namespace
{
    std::set<std::filesystem::path> ChildClassFolders(const std::vector<FNativeClassSource>& _Classes, bool _Engine, const std::filesystem::path& _Folder)
    {
        std::set<std::filesystem::path> Folders;
        for (const auto& Class : _Classes)
        {
            if (Class.__Engine != _Engine) continue;
            auto Folder = Class.__Folder;
            while (!Folder.empty() && Folder.parent_path() != _Folder) Folder = Folder.parent_path();
            if (!Folder.empty() && Folder.parent_path() == _Folder) Folders.insert(Folder);
        }
        return Folders;
    }
}

void FEditorImGui::UpdateContentSources()
{
    auto* Paths = GEngine ? GEngine->GetEngineSystem<PathEngineSystem>() : nullptr;
    if (!Paths) return;
    if (__SourceOpenTask.valid() && __SourceOpenTask.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        auto Result = __SourceOpenTask.get();
        if (Result.first)
            __PendingClassSource.reset();
        else if (ImGui::GetTime() >= __SourceOpenDeadline || (__LaunchSourceEditor && Result.second != "Starting Visual Studio 2022..."))
        {
            __AssetMessage = Result.second;
            __NotificationUntil = ImGui::GetTime() + 8;
            __PendingClassSource.reset();
        }
        __LaunchSourceEditor = false;
        __NextSourceOpenAttempt = ImGui::GetTime() + 0.5;
    }
    if (__PendingClassSource && !__SourceOpenTask.valid() && ImGui::GetTime() >= __NextSourceOpenAttempt)
    {
        const auto Solution = Paths->GetRootDirectory() / L"UnrealEngine.sln";
        const auto Class = *__PendingClassSource;
        const bool Launch = __LaunchSourceEditor;
        __SourceOpenTask = std::async(std::launch::async, [Solution, Class, Launch]
        {
            FString Error;
            try { const bool Opened = OpenNativeClassSource(Solution, Class, Error, Launch); return std::make_pair(Opened, Error); }
            catch (const std::exception&) { return std::make_pair(false, FString(TEXT("Could not open the class source."))); }
        });
    }
    if (!__ContentSettingsLoaded)
    {
        const auto File = Paths->GetProjectSavedDirectory() / L"Config/ContentBrowser.ini";
        __ShowEngineContent = GetPrivateProfileIntW(L"ContentBrowser", L"ShowEngineContent", 1, File.c_str()) != 0;
        __ShowCppClasses = GetPrivateProfileIntW(L"ContentBrowser", L"ShowCppClasses", 1, File.c_str()) != 0;
        __ContentSettingsLoaded = true;
    }
    if (__NativeClassScan.valid())
    {
        if (__NativeClassScan.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            try
            {
                __NativeClasses = __NativeClassScan.get();
                if (__ContentSource == EContentSource::ProjectClasses || __ContentSource == EContentSource::EngineClasses)
                {
                    const bool Engine = __ContentSource == EContentSource::EngineClasses;
                    while (!__ClassFolder.empty())
                    {
                        const bool Exists = std::any_of(__NativeClasses.begin(), __NativeClasses.end(), [&](const auto& _Class)
                        {
                            if (_Class.__Engine != Engine) return false;
                            auto Folder = _Class.__Folder;
                            while (!Folder.empty())
                            {
                                if (Folder == __ClassFolder) return true;
                                Folder = Folder.parent_path();
                            }
                            return false;
                        });
                        if (Exists) break;
                        __ClassFolder = __ClassFolder.parent_path();
                    }
                }
            }
            catch (const std::exception&) { __AssetMessage = "Could not refresh C++ classes."; __NotificationUntil = ImGui::GetTime() + 5; }
            __NextClassRefresh = ImGui::GetTime() + 5;
        }
        return;
    }
    if (ImGui::GetTime() < __NextClassRefresh) return;
    const auto ProjectFile = Paths->GetProjectDirectory() / L"Project.vcxproj";
    const auto EngineFile = Paths->GetEngineDirectory() / L"UE/UE.vcxproj";
    __NativeClassScan = std::async(std::launch::async, [ProjectFile, EngineFile]
    {
        auto Classes = ReadNativeClassSources(ProjectFile, "Project", false);
        auto EngineClasses = ReadNativeClassSources(EngineFile, "UE", true);
        Classes.insert(Classes.end(), EngineClasses.begin(), EngineClasses.end());
        return Classes;
    });
}

void FEditorImGui::DrawContentSourceSettings()
{
    if (ImGui::Button("Settings##ContentBrowser")) ImGui::OpenPopup("Content Browser Settings");
    if (!ImGui::BeginPopup("Content Browser Settings")) return;
    bool Changed = ImGui::MenuItem("Show Engine Content", nullptr, &__ShowEngineContent);
    Changed |= ImGui::MenuItem("Show C++ Classes", nullptr, &__ShowCppClasses);
    if (Changed)
    {
        if ((!__ShowEngineContent && (__ContentSource == EContentSource::EngineContent || __ContentSource == EContentSource::EngineClasses)) ||
            (!__ShowCppClasses && (__ContentSource == EContentSource::ProjectClasses || __ContentSource == EContentSource::EngineClasses)))
        {
            __ContentSource = EContentSource::ProjectContent;
            __ContentBrowserSelection.clear();
        }
        if (auto* Paths = GEngine ? GEngine->GetEngineSystem<PathEngineSystem>() : nullptr)
        {
            const auto File = Paths->GetProjectSavedDirectory() / L"Config/ContentBrowser.ini";
            std::error_code Error;
            std::filesystem::create_directories(File.parent_path(), Error);
            const bool SavedEngine = WritePrivateProfileStringW(L"ContentBrowser", L"ShowEngineContent", __ShowEngineContent ? L"1" : L"0", File.c_str()) != FALSE;
            const bool SavedClasses = WritePrivateProfileStringW(L"ContentBrowser", L"ShowCppClasses", __ShowCppClasses ? L"1" : L"0", File.c_str()) != FALSE;
            if (Error || !SavedEngine || !SavedClasses)
            {
                __AssetMessage = "Could not save Content Browser settings.";
                __NotificationUntil = ImGui::GetTime() + 5;
            }
        }
    }
    ImGui::EndPopup();
}

void FEditorImGui::DrawClassFolders(bool _Engine, const std::filesystem::path& _Folder, int _Depth)
{
    if (_Depth > 32) return;
    const auto Source = _Engine ? EContentSource::EngineClasses : EContentSource::ProjectClasses;
    const auto Children = ChildClassFolders(__NativeClasses, _Engine, _Folder);
    const FString Name = _Folder.empty() ? (_Engine ? "UE" : "Project") : FString(_Folder.filename().wstring()).ToUtf8();
    ImGui::PushID(_Engine ? "EngineClasses" : "ProjectClasses");
    ImGui::PushID(FString(_Folder.wstring()).ToUtf8().c_str());
    const bool Open = ImGui::TreeNodeEx(Name.ToUtf8().c_str(), ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen |
        ImGuiTreeNodeFlags_SpanAvailWidth | (Children.empty() ? ImGuiTreeNodeFlags_Leaf : 0) |
        (__ContentSource == Source && __ClassFolder == _Folder ? ImGuiTreeNodeFlags_Selected : 0));
    if (ImGui::IsItemClicked()) { __ContentSource = Source; __ClassFolder = _Folder; }
    if (Open)
    {
        for (const auto& Child : Children) DrawClassFolders(_Engine, Child, _Depth + 1);
        ImGui::TreePop();
    }
    ImGui::PopID();
    ImGui::PopID();
}

void FEditorImGui::OpenClassSource(const FNativeClassSource& _Class)
{
    auto* Paths = GEngine ? GEngine->GetEngineSystem<PathEngineSystem>() : nullptr;
    if (!Paths) return;
    if (__PendingClassSource) return;
    __PendingClassSource = _Class;
    __LaunchSourceEditor = true;
    __NextSourceOpenAttempt = ImGui::GetTime();
    __SourceOpenDeadline = ImGui::GetTime() + 120;
    __AssetMessage = "Opening source in Visual Studio 2022. Close any Visual Studio dialog to continue.";
    __NotificationUntil = ImGui::GetTime() + 6;
}

void FEditorImGui::DrawNativeClassAssets()
{
    const auto ActorClasses = FWorldPersistence::GetActorClasses();
    const bool Engine = __ContentSource == EContentSource::EngineClasses;
    ImGui::Text("%s / %s / %s", Engine ? "Engine C++ Classes" : "C++ Classes", Engine ? "UE" : "Project", FString(__ClassFolder.wstring()).ToUtf8().c_str());
    ImGui::SetNextItemWidth((std::max)(100.0f, ImGui::GetContentRegionAvail().x - 110));
    ImGui::InputTextWithHint("##ClassSearch", "Search C++ Classes", __AssetSearch, sizeof(__AssetSearch));
    ImGui::SameLine();
    DrawContentSourceSettings();
    ImGui::Separator();
    if (!__ClassFolder.empty() && ImGui::SmallButton("Up")) __ClassFolder = __ClassFolder.parent_path();
    const auto Folders = ChildClassFolders(__NativeClasses, Engine, __ClassFolder);
    const int Columns = (std::max)(1, static_cast<int>(ImGui::GetContentRegionAvail().x / (__ContentBrowserTileSize + 8)));
    if (ImGui::BeginTable("NativeClasses", Columns))
    {
        for (const auto& Folder : Folders)
        {
            ImGui::TableNextColumn();
            const auto Name = FString(Folder.filename().wstring()).ToUtf8();
            ImGui::Button(("[Folder]\n" + Name).c_str(), ImVec2(-1, 85));
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) __ClassFolder = Folder;
        }
        const FString Search(__AssetSearch);
        for (const auto& Class : __NativeClasses)
        {
            if (Class.__Engine != Engine || Class.__Folder != __ClassFolder) continue;
            auto Name = Class.__Name;
            if (!Search.IsEmpty() && !Name.Contains(Search)) continue;
            ImGui::TableNextColumn();
            ImGui::PushID(FString(Class.__Header.wstring()).ToUtf8().c_str());
            ImGui::PushID(Class.__Name.ToUtf8().c_str());
            ImGui::Button((FString(TEXT("C++\n")) + Class.__Name).ToUtf8().c_str(), ImVec2(-1, 85));
            const auto ClassName = Class.__Name.ToUtf8();
            const FString ClassPath = FString("/Script/") + Class.__Module + "." + FString(ClassName.substr(1));
            const bool Placeable = !__IsPlaying && !__WaitingForWorldChange && __ProjectSessionReady &&
                std::find(ActorClasses.begin(), ActorClasses.end(), ClassPath) != ActorClasses.end();
            if (Placeable && ImGui::BeginDragDropSource())
            {
                const auto Path = ClassPath.ToWide();
                ImGui::SetDragDropPayload("NATIVE_ACTOR_CLASS", Path.c_str(), (Path.size() + 1) * sizeof(wchar_t));
                ImGui::TextUnformatted(ClassName.c_str());
                ImGui::EndDragDropSource();
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s\n%s:%d", Class.__Name.ToUtf8().c_str(), FString(Class.__Header.wstring()).ToUtf8().c_str(), Class.__Line);
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) OpenClassSource(Class);
            }
            if (ImGui::BeginPopupContextItem("ClassActions"))
            {
                if (ImGui::MenuItem("Open Source Code")) OpenClassSource(Class);
                if (ImGui::MenuItem("Place in Level", nullptr, false, Placeable)) PlaceNativeActor(ClassPath, 200, 200);
                ImGui::EndPopup();
            }
            ImGui::TextDisabled("C++ Class");
            ImGui::PopID(); ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

void FEditorImGui::PlaceNativeActor(const FString& _ClassPath, float _X, float _Y)
{
    if (__IsPlaying || __WaitingForWorldChange || !__ProjectSessionReady) return;
    auto* Engine = dynamic_cast<UGameEngine*>(GEngine);
    if (!Engine) return;
    Engine->WithWorld([&](UWorld* _World)
    {
        auto* Actor = FWorldPersistence::SpawnActor(_ClassPath, _World);
        if (!Actor) return;
        if (!Actor->GetRootComponent()) Actor->SetRootComponent(Actor->CreateInstanceComponent<USceneComponent>());
        Actor->SetActorLocation(FVector(_X, _Y, 0));
        __SelectedActor = Actor;
        __SelectedComponent = nullptr;
    });
}
