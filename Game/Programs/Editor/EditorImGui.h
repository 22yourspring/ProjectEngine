#pragma once

#include "UE/UnrealString.h"

#include "ThirdParty/ImGui/imgui.h"
#include "UE/InputTypes.h"
#include "AssetImportTask.h"
#include "NativeClassHierarchy.h"
#include "UE/GameMapsSettings.h"

#include <d3d11.h>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <future>
#include <optional>
#pragma push_macro("Super")
#undef Super
#include <wrl/client.h>
#pragma pop_macro("Super")

struct HWND__;
using HWND = HWND__*;

class UObject;
class UAudioComponent;
class UActorComponent;
class UAutoReimportManager;
class AActor;
class UEditorActorSubsystem;

class FEditorImGui final
{
public:
    FEditorImGui();
    ~FEditorImGui();

    FEditorImGui(const FEditorImGui&) = delete;
    FEditorImGui& operator=(const FEditorImGui&) = delete;

    bool Initialize(HWND _MainWindow, HWND _ViewportWindow);
    void Shutdown();
    void Render();
    void Resize(unsigned int _Width, unsigned int _Height);
    void SetEditorState(bool _IsPlaying, bool _IsPaused, bool _UseEngineGameMode);
    bool HandleWindowMessage(
        HWND _Window,
        unsigned int _Message,
        unsigned long long _WParam,
        long long _LParam);
    void HandleFileDrop(void* _Drop);
    void InitializeProjectSession();
    void RequestClose();
    bool UseEngineGameMode() const;

private:
    enum class EFileAction { None, NewLevel, OpenLevel, OpenProject, Exit };
    void RequestFileAction(EFileAction _Action, const std::filesystem::path& _Path = {});
    void ExecuteFileAction();
    bool OpenLevelFile(const std::filesystem::path& _Path);
    bool SaveLevelFile(bool _SaveAs = false);
    void TickProjectSession();
    void DrawFileDialogs();
    void DrawMapSettings();
    void DrawAddActor();
    void RefreshLevelDirty();
    std::filesystem::path ChooseFile(bool _Project, bool _Save);
    void SaveSessionSettings();
    void RegisterProjectFileAction();
    UGameMapsSettings __MapsSettings;
    EFileAction __FileAction = EFileAction::None;
    std::filesystem::path __ActionPath;
    std::vector<uint8> __SavedLevelData;
    int __LevelGameMode = -1;
    int __SavedLevelGameMode = -1;
    bool __ProjectSessionReady = false;
    bool __LevelDirty = false;
    bool __ConfirmFileAction = false;
    bool __LoadLastLevel = false;
    bool __AutoSaveLevels = true;
    int __AutoSaveMinutes = 5;
    double __NextDirtyCheck = 0;
    double __NextAutoSave = 0;
    bool __RecoveryPrompt = false;
    std::filesystem::path __RecoveryOriginal;
    enum class EWindowControl
    {
        Minimize,
        MaximizeRestore,
        Close
    };

    ID3D11ShaderResourceView* LoadIconTexture(const std::filesystem::path& _Path);
    void RevealContentPath(const std::filesystem::path& _Path);
    const char* GetContentIcon(const std::filesystem::directory_entry& _Entry);
    void DrawContentFolder(const std::filesystem::path& _Path, int _Depth = 0);
    bool IsWaitingForAction(const FInputActionKeyMapping& _Mapping);
    bool IsWaitingForAxis(const FInputAxisKeyMapping& _Mapping);
    bool CreateRenderTarget();
    bool CreateD3D11Resources(HWND _Window);
    void ApplyEditorStyle();
    bool DrawWindowControlButton(const char* _ID, EWindowControl _Control);
    void BuildInitialDockLayout(ImGuiID _DockspaceID, const ImVec2& _Size);
    void InitializeLayoutSettings();
    void DrawMainDockspace();
    void DrawWorldOutliner();
    void DrawDetails();
    void PlaceNativeActor(const FString& _ClassPath, float _X, float _Y);
    void DrawGameModeSelector(const char* _Label);
    void DrawWorldSettings();
    void DrawProjectSettings();
    void DrawCollisionSettings();
    void DrawCollisionDetails(class UPrimitiveComponent* _Component);
    void DrawCollisionPreview();
    std::unique_ptr<class UWorld> __CollisionPreviewWorld;
    bool __ShowCollisionPreview = false;
    bool __CollisionPreview3D = false;
    bool __CollisionPreviewRunning = false;
    int __CollisionPreviewHits = 0;
    int __CollisionPreviewOverlaps = 0;
    bool __ProjectSettingsCollisionPage = false;
    FString __CollisionSettingsError;
    bool DrawContentBrowser(float _Height);
    void UpdateContentSources();
    void DrawContentSourceSettings();
    void DrawClassFolders(bool _Engine, const std::filesystem::path& _Folder, int _Depth = 0);
    void DrawNativeClassAssets();
    void OpenClassSource(const FNativeClassSource& _Class);
    void DrawViewport();
    void DrawViewportOptions();
    void BeginAssetImport(const std::filesystem::path& _Destination);
    void OpenAsset(const std::filesystem::path& _Path);
    bool IsEditorMap(const std::filesystem::path& _Path) const;
    void OpenEditorMap(const std::filesystem::path& _Path);
    std::filesystem::path GetEditorActorsSavePath() const;
    void DrawAssetWindows();
    void DrawAssetPreferences();
    void TickAssetWorkflow();
    void SaveEditorAssetActors();
    void QueueImportFiles(const std::vector<std::filesystem::path>& _Files, const std::filesystem::path& _Destination);
    void RunImportTasks(std::vector<UAssetImportTask>& _Tasks);
    void ReimportAsset(const std::filesystem::path& _File, bool _WithNewFile);
    void DrawAssetContextMenu(const std::filesystem::path& _File);
    void DrawAssetDragSource(const std::filesystem::path& _File);
    bool DrawAssetProperty(const char* _Label, UObject*& _Asset, uint32 _Type);
    void DrawMaterialEditor();
    void DrawMaterialCreation(const std::filesystem::path& _Directory);
    bool __ShowMaterialCreate = false;
    UObject* __CreateMaterialParent = nullptr;
    std::filesystem::path __MaterialDirectory;
    char __MaterialName[128] = "M_NewMaterial";
    std::shared_ptr<const struct FMaterialRenderData> __MaterialPreviewData;
    void PlaceAsset(const std::filesystem::path& _File, float _X, float _Y);
    void DrawViewportAssetDrop(const ImVec2& _Position, const ImVec2& _Size);
    ID3D11ShaderResourceView* GetAssetThumbnail(const std::filesystem::path& _File);

    struct FDropTarget { ImVec2 __Min, __Max; std::filesystem::path __Directory; };
    std::vector<FDropTarget> __DropTargets;
    std::vector<UAssetImportTask> __ImportTasks;
    std::unique_ptr<UAutoReimportManager> __AutoReimport;
    std::unique_ptr<UEditorActorSubsystem> __EditorActorSubsystem;
    std::map<std::filesystem::path, std::unique_ptr<UEditorActorSubsystem>> __LevelActorSubsystems;
    std::filesystem::path __EditedMap;
    uint64 __EditorWorldGeneration = 0;
    bool __WaitingForWorldChange = false;
    std::map<FString, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> __AssetThumbnails;
    std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> __RetiredTextures;
    std::vector<std::filesystem::path> __AssetFiles;
    std::map<FString, uint32> __AssetKinds;
    double __NextAssetRefresh = 0;
    double __NotificationUntil = 0;
    bool __ShowAssetPreferences = false;
    AActor* __SelectedActor = nullptr;
    AActor* __DetailsActor = nullptr;
    UActorComponent* __SelectedComponent = nullptr;
    char __DetailsSearch[128] = {};
    char __AssetSearch[128] = {};
    char __PickerSearch[128] = {};
    std::filesystem::path __PreviewPath;
    std::vector<float> __PreviewWaveform;

    HWND __MainWindow = nullptr;
    std::filesystem::path __ImportSource;
    std::filesystem::path __ImportDestination;
    char __ImportAssetName[256] = {};
    bool __ShowImport = false;
    bool __ShowAsset = false;
    FString __AssetMessage;
    UObject* __PreviewAsset = nullptr;
    std::unique_ptr<UAudioComponent> __PreviewAudio;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> __PreviewTexture;
    HWND __ViewportWindow = nullptr;
    ImVec2 __ViewportDesignSize = ImVec2(0.0f, 0.0f);
    bool __ConstrainViewportAspectRatio = false;
    bool __ImmersiveViewport = false;
    bool __ToggleImmersiveRequested = false;
    bool __IsRenderingEditorImGui = false;
    Microsoft::WRL::ComPtr<ID3D11Device> __Device;
    std::map<FString, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> __IconTextures;
    std::filesystem::path __RootDirectory;
    std::filesystem::path __ContentBrowserSelection;
    enum class EContentSource { ProjectContent, EngineContent, ProjectClasses, EngineClasses };
    EContentSource __ContentSource = EContentSource::ProjectContent;
    bool __ShowEngineContent = true;
    bool __ShowCppClasses = true;
    bool __ContentSettingsLoaded = false;
    double __NextClassRefresh = 0;
    std::filesystem::path __ClassFolder;
    std::vector<FNativeClassSource> __NativeClasses;
    std::future<std::vector<FNativeClassSource>> __NativeClassScan;
    std::optional<FNativeClassSource> __PendingClassSource;
    std::future<std::pair<bool, FString>> __SourceOpenTask;
    bool __LaunchSourceEditor = false;
    double __NextSourceOpenAttempt = 0;
    double __SourceOpenDeadline = 0;
    std::filesystem::path __ContentBrowserFocusedPath;
    std::map<FString, bool> __TreeOpenOverrides;
    float __ContentBrowserTileSize = 140.0f;
    float __ContentBrowserSourcesWidth = 210.0f;
    FString __LayoutIniFilename;
    std::vector<char> __LayoutIniUtf8;
    float __ContentDrawerOpenAmount = 0.0f;
    float __ContentDrawerHeight = 320.0f;
    float __ContentDrawerTop = -1.0f;
    int __ViewportClipHeight = -1;
    int __ViewportClipWidth = -1;
    double __ContentDrawerCloseTime = 0.0;
    bool __ContentDrawerOpen = false;
    bool __ContentDrawerPinned = false;
    bool __ContentDrawerShowingOutputLog = false;
    bool __TitleBarDragging = false;
    bool __DockLayoutCreated = false;
    bool __IsPlaying = false;
    bool __IsPaused = false;
    bool __UseEngineGameMode = true;
    bool __ShowWorldSettings = true;
    bool __ShowProjectSettings = false;
    bool __ProjectSettingsInputPage = false;
    bool __WaitingForKey = false;
    bool __WaitingForAxis = false;
    FInputActionKeyMapping __ActionKeyToReplace;
    FInputAxisKeyMapping __AxisKeyToReplace;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> __DeviceContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain> __SwapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> __RenderTargetView;

};
