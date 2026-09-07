#pragma once

#include "ThirdParty/ImGui/imgui.h"
#include "UE/InputTypes.h"

#include <d3d11.h>
#include <filesystem>
#include <map>
#include <string>
#pragma push_macro("Super")
#undef Super
#include <wrl/client.h>
#pragma pop_macro("Super")

struct HWND__;
using HWND = HWND__*;

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

private:
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
    void DrawMainDockspace();
    void DrawWorldOutliner();
    void DrawDetails();
    void DrawGameModeSelector(const char* _Label);
    void DrawWorldSettings();
    void DrawProjectSettings();
    bool DrawContentBrowser(float _Height);
    void DrawViewport();

    HWND __MainWindow = nullptr;
    HWND __ViewportWindow = nullptr;
    ImVec2 __ViewportDesignSize = ImVec2(0.0f, 0.0f);
    bool __ConstrainViewportAspectRatio = false;
    bool __IsRenderingEditorImGui = false;
    Microsoft::WRL::ComPtr<ID3D11Device> __Device;
    std::map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> __IconTextures;
    std::filesystem::path __ContentBrowserSelection;
    std::filesystem::path __ContentBrowserFocusedPath;
    std::map<std::string, bool> __TreeOpenOverrides;
    float __ContentBrowserTileSize = 140.0f;
    float __ContentDrawerOpenAmount = 0.0f;
    float __ContentDrawerHeight = 320.0f;
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
