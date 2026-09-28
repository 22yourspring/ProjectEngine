#include "framework.h"
#include "EditorImGui.h"
#include "UE/World.h"
#include "Resource.h"
#include "UE/CoreMinimal.h"
#include "GameModuleAccess.h"
#include "UE/PathEngineSystem.h"
#include "UE/InputEngineSystem.h"
#include "UE/ResourceEngineSystem.h"
#include "UE/Texture2D.h"
#include "UE/SoundWave.h"
#include "UE/AudioComponent.h"
#include "AssetImport.h"
#include "AutoReimportManager.h"
#include "EditorActorSubsystem.h"
#include <commdlg.h>
#pragma comment(lib, "comdlg32.lib")

#include "ThirdParty/ImGui/imgui.h"
#include "ThirdParty/ImGui/imgui_internal.h"
#include "ThirdParty/ImGui/backends/imgui_impl_dx11.h"
#include "ThirdParty/ImGui/backends/imgui_impl_win32.h"

#include <d3d11.h>
#define NANOSVG_IMPLEMENTATION
#include "ThirdParty/NanoSVG/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "ThirdParty/NanoSVG/nanosvgrast.h"
#include <algorithm>
#include <filesystem>
#include <iterator>
#include <map>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <tuple>
#include <chrono>
#include <fstream>
#include <Windows.h>
#include <shellapi.h>
#include "ThirdParty/WIL/include/wil/resource.h"
#pragma push_macro("Super")
#undef Super
#include <wrl/client.h>
#pragma pop_macro("Super")

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Shell32.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND _Window, UINT _Message, WPARAM _WParam, LPARAM _LParam);

ID3D11ShaderResourceView* FEditorImGui::LoadIconTexture(const std::filesystem::path& _Path)
{
    const FString Key(_Path.wstring());
    auto Found = __IconTextures.find(Key);
    if (Found != __IconTextures.end()) return Found->second.Get();
    NSVGimage* Image = nsvgParseFromFile(_Path.string().c_str(), "px", 96.0f);
    if (!Image) return nullptr;
    const int Width = static_cast<int>(Image->width), Height = static_cast<int>(Image->height);
    std::vector<unsigned char> Pixels(static_cast<size_t>(Width * Height * 4));
    NSVGrasterizer* Raster = nsvgCreateRasterizer();
    nsvgRasterize(Raster, Image, 0, 0, 1, Pixels.data(), Width, Height, Width * 4);
    nsvgDeleteRasterizer(Raster); nsvgDelete(Image);
    D3D11_TEXTURE2D_DESC Desc = {}; Desc.Width = Width; Desc.Height = Height;
    Desc.MipLevels = 1; Desc.ArraySize = 1; Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    Desc.SampleDesc.Count = 1; Desc.Usage = D3D11_USAGE_IMMUTABLE; Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA Data = { Pixels.data(), static_cast<UINT>(Width * 4), 0 };
    Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture;
    if (FAILED(__Device->CreateTexture2D(&Desc, &Data, Texture.GetAddressOf()))) return nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> View;
    if (FAILED(__Device->CreateShaderResourceView(Texture.Get(), nullptr, View.GetAddressOf()))) return nullptr;
    return __IconTextures.emplace(Key, std::move(View)).first->second.Get();
}

void FEditorImGui::RevealContentPath(const std::filesystem::path& _Path)
{
    std::filesystem::path Current = _Path;
    while (!Current.empty())
    {
        __TreeOpenOverrides[FString(Current.wstring()).ToUtf8()] = true;
        const std::filesystem::path Parent = Current.parent_path();
        if (Parent == Current)
            break;
        Current = Parent;
    }
}

const char* FEditorImGui::GetContentIcon(const std::filesystem::directory_entry& _Entry)
{
    if (_Entry.is_directory())
        return "[DIR]";
    const FString Extension = _Entry.path().extension().wstring();
    if (_wcsicmp(Extension.c_str(), TEXT(".cpp")) == 0 ||
        _wcsicmp(Extension.c_str(), TEXT(".h")) == 0 ||
        _wcsicmp(Extension.c_str(), TEXT(".inl")) == 0)
        return "[C++]";
    if (_wcsicmp(Extension.c_str(), TEXT(".umap")) == 0)
        return "[MAP]";
    if (_wcsicmp(Extension.c_str(), TEXT(".uasset")) == 0)
        return "[ASSET]";
    if (_wcsicmp(Extension.c_str(), TEXT(".ini")) == 0 ||
        _wcsicmp(Extension.c_str(), TEXT(".json")) == 0)
        return "[CFG]";
    if (_wcsicmp(Extension.c_str(), TEXT(".txt")) == 0 ||
        _wcsicmp(Extension.c_str(), TEXT(".md")) == 0)
        return "[TXT]";
    return "[FILE]";
}

void FEditorImGui::DrawContentFolder(const std::filesystem::path& _Path, int _Depth)
{
    if (_Depth > 8)
        return;
    std::error_code Error;
    for (const auto& Entry : std::filesystem::directory_iterator(_Path, Error))
    {
        if (Error || !Entry.is_directory(Error))
            continue;
        const FString Name = FString(Entry.path().filename().wstring()).ToUtf8();
        ImGui::PushID(FString(Entry.path().wstring()).ToUtf8().c_str());
        const ImGuiTreeNodeFlags SelectionFlag =
            __ContentBrowserFocusedPath == Entry.path() ? ImGuiTreeNodeFlags_Selected : 0;
        const FString PathKey = FString(Entry.path().wstring()).ToUtf8();
        const auto OpenOverride = __TreeOpenOverrides.find(PathKey);
        if (OpenOverride != __TreeOpenOverrides.end())
            ImGui::SetNextItemOpen(OpenOverride->second);
        const bool IsOpen = ImGui::TreeNodeEx(
            Name.ToUtf8().c_str(),
            ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnDoubleClick |
            SelectionFlag);
        __DropTargets.push_back({ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), Entry.path()});
        const bool DoubleClicked = ImGui::IsItemHovered() &&
            ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            __ContentBrowserFocusedPath = Entry.path();
            __ContentBrowserSelection = Entry.path();
        }
        if (DoubleClicked && IsOpen)
        {
            // An already-open folder is entered on double click, not
            // collapsed as a side effect of the second click.
            __ContentBrowserSelection = Entry.path();
            __TreeOpenOverrides[PathKey] = true;
        }
        else if (DoubleClicked)
            __ContentBrowserSelection = Entry.path();
        else if (IsOpen && ImGui::IsItemClicked(ImGuiMouseButton_Left))
            __ContentBrowserSelection = Entry.path();
        if (IsOpen)
        {
            DrawContentFolder(Entry.path(), _Depth + 1);
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}

bool FEditorImGui::IsWaitingForAction(const FInputActionKeyMapping& _Mapping)
{
    return __WaitingForKey &&
        _Mapping.__ActionName == __ActionKeyToReplace.__ActionName &&
        _Mapping.__Key == __ActionKeyToReplace.__Key;
}

bool FEditorImGui::IsWaitingForAxis(const FInputAxisKeyMapping& _Mapping)
{
    return __WaitingForAxis &&
        _Mapping.__AxisName == __AxisKeyToReplace.__AxisName &&
        _Mapping.__Key == __AxisKeyToReplace.__Key &&
        _Mapping.__Scale == __AxisKeyToReplace.__Scale;
}

bool FEditorImGui::CreateRenderTarget()
{
    Microsoft::WRL::ComPtr<ID3D11Texture2D> BackBuffer;
    if (FAILED(__SwapChain->GetBuffer(0, IID_PPV_ARGS(BackBuffer.ReleaseAndGetAddressOf()))))
        return false;

    return SUCCEEDED(__Device->CreateRenderTargetView(
        BackBuffer.Get(), nullptr, __RenderTargetView.ReleaseAndGetAddressOf()));
}

bool FEditorImGui::CreateD3D11Resources(HWND _Window)
{
    DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
    SwapChainDesc.BufferCount = 2;
    SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.OutputWindow = _Window;
    SwapChainDesc.SampleDesc.Count = 1;
    SwapChainDesc.Windowed = TRUE;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    constexpr D3D_FEATURE_LEVEL FeatureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    D3D_FEATURE_LEVEL CreatedFeatureLevel = D3D_FEATURE_LEVEL_10_0;
    HRESULT Result = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        FeatureLevels,
        static_cast<UINT>(std::size(FeatureLevels)),
        D3D11_SDK_VERSION,
        &SwapChainDesc,
        __SwapChain.ReleaseAndGetAddressOf(),
        __Device.ReleaseAndGetAddressOf(),
        &CreatedFeatureLevel,
        __DeviceContext.ReleaseAndGetAddressOf());

    if (E_INVALIDARG == Result)
    {
        Result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            0,
            FeatureLevels + 1,
            static_cast<UINT>(std::size(FeatureLevels) - 1),
            D3D11_SDK_VERSION,
            &SwapChainDesc,
            __SwapChain.ReleaseAndGetAddressOf(),
            __Device.ReleaseAndGetAddressOf(),
            &CreatedFeatureLevel,
            __DeviceContext.ReleaseAndGetAddressOf());
    }

    return SUCCEEDED(Result) && CreateRenderTarget();
}

void FEditorImGui::ApplyEditorStyle()
{
    ImGui::StyleColorsDark();
    ImGuiStyle& Style = ImGui::GetStyle();

    // Unreal Engine 5.4 Starship palette (SlateCore/Private/Styling/StyleColors.cpp).
    const ImVec4 Black = ImVec4(0.000f, 0.000f, 0.000f, 1.000f);
    const ImVec4 Title = ImVec4(0.082f, 0.082f, 0.082f, 1.000f);            // #151515
    const ImVec4 Input = ImVec4(0.059f, 0.059f, 0.059f, 1.000f);            // #0F0F0F
    const ImVec4 Recessed = ImVec4(0.102f, 0.102f, 0.102f, 1.000f);         // #1A1A1A
    const ImVec4 Panel = ImVec4(0.141f, 0.141f, 0.141f, 1.000f);            // #242424
    const ImVec4 Header = ImVec4(0.184f, 0.184f, 0.184f, 1.000f);           // #2F2F2F
    const ImVec4 Dropdown = ImVec4(0.220f, 0.220f, 0.220f, 1.000f);         // #383838
    const ImVec4 DropdownOutline = ImVec4(0.298f, 0.298f, 0.298f, 1.000f);  // #4C4C4C
    const ImVec4 Hover = ImVec4(0.341f, 0.341f, 0.341f, 1.000f);            // #575757
    const ImVec4 Hover2 = ImVec4(0.502f, 0.502f, 0.502f, 1.000f);           // #808080
    const ImVec4 Foreground = ImVec4(0.753f, 0.753f, 0.753f, 1.000f);       // #C0C0C0
    const ImVec4 ForegroundHover = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
    const ImVec4 Primary = ImVec4(0.000f, 0.439f, 0.878f, 1.000f);          // #0070E0
    const ImVec4 PrimaryHover = ImVec4(0.055f, 0.525f, 1.000f, 1.000f);     // #0E86FF
    const ImVec4 PrimaryPress = ImVec4(0.000f, 0.314f, 0.627f, 1.000f);     // #0050A0

    Style.WindowRounding = 0.0f;
    Style.ChildRounding = 0.0f;
    Style.FrameRounding = 3.0f;
    Style.TabRounding = 0.0f;
    Style.WindowBorderSize = 1.0f;
    Style.FrameBorderSize = 1.0f;
    Style.PopupBorderSize = 1.0f;
    Style.PopupRounding = 3.0f;
    Style.ScrollbarRounding = 4.0f;
    Style.GrabRounding = 3.0f;
    Style.GrabMinSize = 8.0f;
    Style.ScrollbarSize = 12.0f;
    Style.ItemSpacing = ImVec2(6.0f, 5.0f);
    Style.ItemInnerSpacing = ImVec2(5.0f, 4.0f);
    Style.WindowPadding = ImVec2(6.0f, 6.0f);
    Style.FramePadding = ImVec2(6.0f, 4.0f);
    Style.TabBarBorderSize = 1.0f;
    Style.TabBarOverlineSize = 2.0f;

    Style.Colors[ImGuiCol_Text] = Foreground;
    Style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.400f, 0.400f, 0.400f, 1.000f);
    Style.Colors[ImGuiCol_WindowBg] = Recessed;
    Style.Colors[ImGuiCol_ChildBg] = Panel;
    Style.Colors[ImGuiCol_PopupBg] = Dropdown;
    Style.Colors[ImGuiCol_Border] = DropdownOutline;
    Style.Colors[ImGuiCol_BorderShadow] = Black;
    Style.Colors[ImGuiCol_FrameBg] = Input;
    Style.Colors[ImGuiCol_FrameBgHovered] = Header;
    Style.Colors[ImGuiCol_FrameBgActive] = Dropdown;
    Style.Colors[ImGuiCol_TitleBg] = Title;
    Style.Colors[ImGuiCol_TitleBgActive] = Panel;
    Style.Colors[ImGuiCol_TitleBgCollapsed] = Title;
    Style.Colors[ImGuiCol_MenuBarBg] = Title;
    Style.Colors[ImGuiCol_ScrollbarBg] = Recessed;
    Style.Colors[ImGuiCol_ScrollbarGrab] = Dropdown;
    Style.Colors[ImGuiCol_ScrollbarGrabHovered] = Hover;
    Style.Colors[ImGuiCol_ScrollbarGrabActive] = Hover2;
    Style.Colors[ImGuiCol_CheckMark] = ForegroundHover;
    Style.Colors[ImGuiCol_SliderGrab] = Hover2;
    Style.Colors[ImGuiCol_SliderGrabActive] = PrimaryHover;
    Style.Colors[ImGuiCol_Button] = Header;
    Style.Colors[ImGuiCol_ButtonHovered] = Hover;
    Style.Colors[ImGuiCol_ButtonActive] = Dropdown;
    Style.Colors[ImGuiCol_Header] = Header;
    Style.Colors[ImGuiCol_HeaderHovered] = Hover;
    Style.Colors[ImGuiCol_HeaderActive] = Dropdown;
    Style.Colors[ImGuiCol_Separator] = Input;
    Style.Colors[ImGuiCol_SeparatorHovered] = DropdownOutline;
    Style.Colors[ImGuiCol_SeparatorActive] = Primary;
    Style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
    Style.Colors[ImGuiCol_ResizeGripHovered] = PrimaryHover;
    Style.Colors[ImGuiCol_ResizeGripActive] = PrimaryPress;
    Style.Colors[ImGuiCol_Tab] = Title;
    Style.Colors[ImGuiCol_TabHovered] = Header;
    Style.Colors[ImGuiCol_TabSelected] = Panel;
    Style.Colors[ImGuiCol_TabSelectedOverline] = Primary;
    Style.Colors[ImGuiCol_TabDimmed] = Title;
    Style.Colors[ImGuiCol_TabDimmedSelected] = Recessed;
    Style.Colors[ImGuiCol_TabDimmedSelectedOverline] = DropdownOutline;
    Style.Colors[ImGuiCol_DockingPreview] = ImVec4(Primary.x, Primary.y, Primary.z, 0.700f);
    Style.Colors[ImGuiCol_DockingEmptyBg] = Title;
    Style.Colors[ImGuiCol_TableHeaderBg] = Header;
    Style.Colors[ImGuiCol_TableBorderStrong] = Input;
    Style.Colors[ImGuiCol_TableBorderLight] = Recessed;
    Style.Colors[ImGuiCol_TextLink] = PrimaryHover;
    Style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(Primary.x, Primary.y, Primary.z, 0.550f);
    Style.Colors[ImGuiCol_DragDropTarget] = PrimaryHover;
    Style.Colors[ImGuiCol_NavCursor] = PrimaryHover;
    Style.Colors[ImGuiCol_NavWindowingHighlight] = ForegroundHover;
    Style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.450f);
    Style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.600f);
}

bool FEditorImGui::DrawWindowControlButton(const char* _ID, EWindowControl _Control)
{
    constexpr float ButtonWidth = 38.0f;
    const float ButtonHeight = ImGui::GetFrameHeight();
    const bool Clicked = ImGui::InvisibleButton(_ID, ImVec2(ButtonWidth, ButtonHeight));
    const bool Hovered = ImGui::IsItemHovered();
    const bool Held = ImGui::IsItemActive();
    const ImVec2 Minimum = ImGui::GetItemRectMin();
    const ImVec2 Maximum = ImGui::GetItemRectMax();
    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    if (Hovered || Held)
    {
        const ImU32 Background = EWindowControl::Close == _Control
            ? IM_COL32(196, 43, 28, Held ? 255 : 230)
            : ImGui::GetColorU32(Held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
        DrawList->AddRectFilled(Minimum, Maximum, Background);
    }

    const ImU32 Foreground = ImGui::GetColorU32(
        Hovered ? ImGuiCol_Text : ImGuiCol_TextDisabled);
    const ImVec2 Center(
        (Minimum.x + Maximum.x) * 0.5f,
        (Minimum.y + Maximum.y) * 0.5f);

    if (EWindowControl::Minimize == _Control)
    {
        DrawList->AddLine(
            ImVec2(Center.x - 5.0f, Center.y + 3.0f),
            ImVec2(Center.x + 5.0f, Center.y + 3.0f),
            Foreground, 1.0f);
    }
    else if (EWindowControl::MaximizeRestore == _Control)
    {
        if (IsZoomed(__MainWindow))
        {
            DrawList->AddRect(
                ImVec2(Center.x - 3.0f, Center.y - 5.0f),
                ImVec2(Center.x + 5.0f, Center.y + 3.0f), Foreground);
            DrawList->AddRectFilled(
                ImVec2(Center.x - 5.0f, Center.y - 3.0f),
                ImVec2(Center.x + 3.0f, Center.y + 5.0f),
                ImGui::GetColorU32(ImGuiCol_MenuBarBg));
            DrawList->AddRect(
                ImVec2(Center.x - 5.0f, Center.y - 3.0f),
                ImVec2(Center.x + 3.0f, Center.y + 5.0f), Foreground);
        }
        else
        {
            DrawList->AddRect(
                ImVec2(Center.x - 5.0f, Center.y - 5.0f),
                ImVec2(Center.x + 5.0f, Center.y + 5.0f), Foreground);
        }
    }
    else
    {
        DrawList->AddLine(
            ImVec2(Center.x - 5.0f, Center.y - 5.0f),
            ImVec2(Center.x + 5.0f, Center.y + 5.0f), Foreground, 1.0f);
        DrawList->AddLine(
            ImVec2(Center.x + 5.0f, Center.y - 5.0f),
            ImVec2(Center.x - 5.0f, Center.y + 5.0f), Foreground, 1.0f);
    }

    return Clicked;
}

void FEditorImGui::BuildInitialDockLayout(ImGuiID _DockspaceID, const ImVec2& _Size)
{
    ImGui::DockBuilderRemoveNode(_DockspaceID);
    ImGui::DockBuilderAddNode(_DockspaceID, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(_DockspaceID, _Size);

    ImGuiID Main = _DockspaceID;
    ImGuiID Left = ImGui::DockBuilderSplitNode(Main, ImGuiDir_Left, 0.18f, nullptr, &Main);
    ImGuiID Right = ImGui::DockBuilderSplitNode(Main, ImGuiDir_Right, 0.22f, nullptr, &Main);
    ImGuiID Bottom = ImGui::DockBuilderSplitNode(Main, ImGuiDir_Down, 0.27f, nullptr, &Main);

    ImGui::DockBuilderDockWindow("World Outliner", Left);
    ImGui::DockBuilderDockWindow("Details", Right);
    ImGui::DockBuilderDockWindow("World Settings", Right);
    ImGui::DockBuilderDockWindow("Viewport", Main);
    ImGui::DockBuilderFinish(_DockspaceID);
}

void FEditorImGui::DrawMainDockspace()
{
    const ImGuiViewport* MainViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(MainViewport->WorkPos);
    ImGui::SetNextWindowSize(MainViewport->WorkSize);
    ImGui::SetNextWindowViewport(MainViewport->ID);

    constexpr ImGuiWindowFlags HostFlags =
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("UnrealEngine Editor", nullptr, HostFlags);
    ImGui::PopStyleVar(3);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Level", "Ctrl+N")) RequestFileAction(EFileAction::NewLevel);
            if (ImGui::MenuItem("Open Level...", "Ctrl+O")) RequestFileAction(EFileAction::OpenLevel);
            if (ImGui::MenuItem("Save Current Level", "Ctrl+S")) SaveLevelFile();
            if (ImGui::MenuItem("Save Current Level As...", "Ctrl+Alt+S")) SaveLevelFile(true);
            if (ImGui::MenuItem("Save All", "Ctrl+Shift+S")) SaveEditorAssetActors();
            ImGui::Separator();
            if (ImGui::MenuItem("Open Project...")) RequestFileAction(EFileAction::OpenProject);
            if (ImGui::MenuItem("Register Project File Action")) RegisterProjectFileAction();
            ImGui::Separator();
            if (ImGui::MenuItem("Exit"))
                PostMessageW(__MainWindow, WM_CLOSE, 0, 0);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            ImGui::BeginDisabled();
            ImGui::MenuItem("Undo", "Ctrl+Z");
            ImGui::MenuItem("Redo", "Ctrl+Y");
            ImGui::EndDisabled();
            ImGui::Separator();
            if (ImGui::BeginMenu("Editor Preferences"))
            {
                if (ImGui::MenuItem("Loading & Saving...")) __ShowAssetPreferences = true;
                if (ImGui::MenuItem("Keyboard Shortcuts..."))
                {
                    __ShowProjectSettings = true;
                    __ProjectSettingsInputPage = true;
                    __ProjectSettingsCollisionPage = false;
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Project Settings..."))
            {
                __ShowProjectSettings = true;
                __ProjectSettingsInputPage = false;
                __ProjectSettingsCollisionPage = false;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window"))
        {
            ImGui::MenuItem("World Outliner", nullptr, true);
            ImGui::MenuItem("Details", nullptr, true);
            if (ImGui::MenuItem("Content Drawer", "Ctrl+Space"))
            {
                __ContentDrawerOpen = true;
                __ContentDrawerCloseTime = ImGui::GetTime() + 0.35;
            }
            ImGui::MenuItem("Output Log", nullptr, true);
            ImGui::Separator();
            ImGui::MenuItem("World Settings", nullptr, &__ShowWorldSettings);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools"))
        {
            ImGui::BeginDisabled();
            ImGui::MenuItem("Debug");
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Build"))
        {
            ImGui::BeginDisabled();
            ImGui::MenuItem("Build All Levels");
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Select"))
        {
            ImGui::BeginDisabled();
            ImGui::MenuItem("Select All", "Ctrl+A");
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Actor"))
        {
            ImGui::BeginDisabled();
            ImGui::MenuItem("Add Actor");
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("About..."))
                ImGui::OpenPopup("AboutUnrealEngine");
            ImGui::EndMenu();
        }

        constexpr float WindowButtonWidth = 38.0f;
        ImGui::SameLine(0.0f, 0.0f);
        const float WindowControlsStart =
            ImGui::GetWindowWidth() - WindowButtonWidth * 3.0f;
        const float AvailableDragWidth =
            WindowControlsStart - ImGui::GetCursorPosX();
        const float DragWidth = AvailableDragWidth > 1.0f ? AvailableDragWidth : 1.0f;
        ImGui::InvisibleButton(
            "##EditorTitleBarDrag", ImVec2(DragWidth, ImGui::GetFrameHeight()));
        const ImVec2 DragMinimum = ImGui::GetItemRectMin();
        const ImVec2 DragMaximum = ImGui::GetItemRectMax();
        wchar_t NativeTitle[1024] = {}; GetWindowTextW(__MainWindow, NativeTitle, 1024);
        const auto TitleText = FString(NativeTitle).ToUtf8();
        const char* WindowTitle = TitleText.c_str();
        const ImVec2 TitleSize = ImGui::CalcTextSize(WindowTitle);
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(
                (DragMinimum.x + DragMaximum.x - TitleSize.x) * 0.5f,
                (DragMinimum.y + DragMaximum.y - TitleSize.y) * 0.5f),
            ImGui::GetColorU32(ImGuiCol_TextDisabled),
            WindowTitle);

        if (ImGui::IsItemHovered() &&
            ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            ShowWindow(__MainWindow, IsZoomed(__MainWindow) ? SW_RESTORE : SW_MAXIMIZE);
        }
        else if (ImGui::IsItemHovered() &&
            ImGui::IsMouseDragging(ImGuiMouseButton_Left, 4.0f) &&
            false == __TitleBarDragging)
        {
            __TitleBarDragging = true;
            ReleaseCapture();
            PostMessageW(__MainWindow, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        }
        else if (false == ImGui::IsMouseDown(ImGuiMouseButton_Left))
            __TitleBarDragging = false;

        ImGui::SameLine(0.0f, 0.0f);
        if (DrawWindowControlButton("##EditorMinimize", EWindowControl::Minimize))
            PostMessageW(__MainWindow, WM_SYSCOMMAND, SC_MINIMIZE, 0);
        ImGui::SameLine(0.0f, 0.0f);
        if (DrawWindowControlButton("##EditorMaximize", EWindowControl::MaximizeRestore))
        {
            PostMessageW(
                __MainWindow,
                WM_SYSCOMMAND,
                IsZoomed(__MainWindow) ? SC_RESTORE : SC_MAXIMIZE,
                0);
        }
        ImGui::SameLine(0.0f, 0.0f);
        if (DrawWindowControlButton("##EditorClose", EWindowControl::Close))
            PostMessageW(__MainWindow, WM_CLOSE, 0, 0);

        ImGui::EndMenuBar();
    }

    if (ImGui::BeginPopupModal("AboutUnrealEngine", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("UnrealEngine Editor");
        ImGui::TextDisabled("Dear ImGui editor interface");
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(90.0f, 0.0f)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    constexpr float ToolbarHeight = 42.0f;
    ImGui::BeginChild("LevelEditorToolbar", ImVec2(0.0f, ToolbarHeight),
        ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar);
    ImGui::SetCursorPosY(8.0f);
    DrawAddActor();
    ImGui::SameLine();
    if (ImGui::Button("Blueprints"))
        ImGui::OpenPopup("BlueprintsMenu");
    if (ImGui::BeginPopup("BlueprintsMenu"))
    {
        ImGui::BeginDisabled();
        ImGui::MenuItem("Open Level Blueprint");
        ImGui::MenuItem("Open Blueprint Class...");
        ImGui::EndDisabled();
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    ImGui::BeginDisabled();
    ImGui::Button("Cinematics");
    ImGui::SameLine();
    ImGui::Button("Build");
    ImGui::EndDisabled();

    ImGui::SameLine(ImGui::GetWindowWidth() * 0.48f);
    const std::filesystem::path ToolbarContent = __RootDirectory / "Engine" / "Content";
    ImGui::BeginDisabled(__IsPlaying);
    ID3D11ShaderResourceView* PlayIcon = LoadIconTexture(ToolbarContent / "Play.svg");
    if (PlayIcon && ImGui::ImageButton("##Play", reinterpret_cast<ImTextureID>(PlayIcon), ImVec2(22.0f, 22.0f)))
        PostMessageW(__MainWindow, WM_COMMAND, IDM_PLAY, 0);
    else if (!PlayIcon && ImGui::Button("> Play"))
        PostMessageW(__MainWindow, WM_COMMAND, IDM_PLAY, 0);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Play (PIE)");
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(false == __IsPlaying);
    ID3D11ShaderResourceView* PauseIcon = LoadIconTexture(
        ToolbarContent / (__IsPaused ? "Resume.svg" : "Pause.svg"));
    if (PauseIcon)
    {
        if (ImGui::ImageButton(
            "##PauseResume", reinterpret_cast<ImTextureID>(PauseIcon), ImVec2(22.0f, 22.0f)))
        {
            PostMessageW(__MainWindow, WM_COMMAND, IDM_PAUSE, 0);
        }
    }
    else
    {
        if (ImGui::Button(__IsPaused ? "Resume" : "Pause"))
            PostMessageW(__MainWindow, WM_COMMAND, IDM_PAUSE, 0);
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(__IsPaused ? "Resume" : "Pause");
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(false == __IsPlaying);
    ID3D11ShaderResourceView* StopIcon = LoadIconTexture(ToolbarContent / "Stop.svg");
    if (StopIcon && ImGui::ImageButton("##Stop", reinterpret_cast<ImTextureID>(StopIcon), ImVec2(22.0f, 22.0f)))
        PostMessageW(__MainWindow, WM_COMMAND, IDM_STOP, 0);
    else if (!StopIcon)
        ImGui::Button("Stop");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Stop");
    ImGui::EndDisabled();

    ImGui::SameLine(ImGui::GetWindowWidth() - 80.0f);
    if (ImGui::Button("Settings"))
        ImGui::OpenPopup("ViewportSettings");
    if (ImGui::BeginPopup("ViewportSettings"))
    {
        if (ImGui::MenuItem("Immersive Mode", "F11", __ImmersiveViewport))
            __ToggleImmersiveRequested = true;
        ImGui::TextUnformatted("Viewport Options");
        ImGui::Separator();
        ImGui::Checkbox("Constrain Aspect Ratio", &__ConstrainViewportAspectRatio);
        ImGui::TextDisabled("When disabled, the viewport fills the panel.");
        ImGui::EndPopup();
    }
    ImGui::EndChild();

    ImGuiIO& IO = ImGui::GetIO();
    if (IO.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Space, false))
    {
        __ContentDrawerOpen = false == __ContentDrawerOpen;
        __ContentDrawerCloseTime = ImGui::GetTime() + 0.35;
    }

    constexpr float StatusBarHeight = 27.0f;
    constexpr float DrawerResizeHandleHeight = 6.0f;
    if (__ContentDrawerPinned)
        __ContentDrawerOpen = true;
    const ImVec2 AvailableSize = ImGui::GetContentRegionAvail();
    const float MinimumDrawerHeight = (std::min)(220.0f,
        (std::max)(40.0f, AvailableSize.y - StatusBarHeight - DrawerResizeHandleHeight - 1.0f));
    const float MaximumDrawerHeight = (std::clamp)(
        AvailableSize.y * 0.38f,
        MinimumDrawerHeight,
        (std::max)(MinimumDrawerHeight, 460.0f));
    const float TargetOpenAmount = __ContentDrawerOpen ? 1.0f : 0.0f;
    const float AnimationStep = (std::min)(1.0f, IO.DeltaTime * 10.0f);
    __ContentDrawerOpenAmount +=
        (TargetOpenAmount - __ContentDrawerOpenAmount) * AnimationStep;
    if (0.001f > __ContentDrawerOpenAmount)
        __ContentDrawerOpenAmount = 0.0f;
    else if (0.999f < __ContentDrawerOpenAmount)
        __ContentDrawerOpenAmount = 1.0f;

    __ContentDrawerHeight = (std::clamp)(__ContentDrawerHeight, MinimumDrawerHeight, MaximumDrawerHeight);
    const float DrawerHeight = __ContentDrawerHeight * __ContentDrawerOpenAmount;
    const float ResizeHandleHeight = 0.001f < __ContentDrawerOpenAmount
        ? DrawerResizeHandleHeight : 0.0f;
    const float DockspaceHeight = (std::max)(
        1.0f,
        AvailableSize.y - StatusBarHeight);
    const ImVec2 DockspacePosition = ImGui::GetCursorScreenPos();
    const ImGuiID DockspaceID = ImGui::GetID("UnrealEngineDockspace");
    if (false == __DockLayoutCreated)
    {
        if (!ImGui::DockBuilderGetNode(DockspaceID))
            BuildInitialDockLayout(DockspaceID, ImVec2(AvailableSize.x, DockspaceHeight));
        __DockLayoutCreated = true;
    }
    ImGui::DockSpace(DockspaceID, ImVec2(0.0f, DockspaceHeight));

    bool DrawerHovered = false;
    bool ResizeHandleHovered = false;
    bool ResizeHandleActive = false;
    __ContentDrawerTop = -1.0f;
    if (ResizeHandleHeight > 0.0f)
    {
        __ContentDrawerTop = DockspacePosition.y + DockspaceHeight - DrawerHeight - ResizeHandleHeight;
        ImGui::SetNextWindowPos(ImVec2(DockspacePosition.x, __ContentDrawerTop));
        ImGui::SetNextWindowSize(ImVec2(AvailableSize.x, DrawerHeight + ResizeHandleHeight));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Content Drawer Overlay", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoFocusOnAppearing);
        if (!ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
            ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
    }
    if (ResizeHandleHeight > 0.0f)
    {
        ImGui::InvisibleButton("ContentDrawerResizeHandle",
            ImVec2(-1.0f, ResizeHandleHeight));
        ResizeHandleHovered = ImGui::IsItemHovered();
        ResizeHandleActive = ImGui::IsItemActive();
        if (ResizeHandleHovered || ResizeHandleActive)
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        if (ResizeHandleActive)
            __ContentDrawerHeight -= IO.MouseDelta.y;
        __ContentDrawerHeight = (std::clamp)(__ContentDrawerHeight, MinimumDrawerHeight, MaximumDrawerHeight);
        const ImVec2 HandleMinimum = ImGui::GetItemRectMin();
        const ImVec2 HandleMaximum = ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddRectFilled(
            HandleMinimum, HandleMaximum, ImGui::GetColorU32(ImGuiCol_Separator));
    }
    if (DrawerHeight > 1.0f)
        DrawerHovered = DrawContentBrowser(DrawerHeight);
    if (ResizeHandleHeight > 0.0f)
    {
        ImGui::End();
        ImGui::PopStyleVar(2);
    }

    const float PreviousStatusBarY = ImGui::GetCursorPosY();
    ImGui::SetCursorPosY((std::max)(0.0f, PreviousStatusBarY - 10.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.082f, 0.082f, 0.082f, 1.0f));
    ImGui::BeginChild(
        "ContentDrawerStatusBar",
        ImVec2(0.0f, StatusBarHeight),
        ImGuiChildFlags_Borders,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    const ImVec2 StatusBarMinimum = ImGui::GetWindowPos();
    const ImVec2 StatusBarSize = ImGui::GetWindowSize();
    const ImVec2 StatusBarMaximum(
        StatusBarMinimum.x + StatusBarSize.x,
        StatusBarMinimum.y + StatusBarSize.y);
    const float StatusButtonY = (StatusBarHeight - 21.0f) * 0.5f;
    ImGui::SetCursorPos(ImVec2(8.0f, StatusButtonY));
    if (ImGui::Button("Content Browser", ImVec2(132.0f, 21.0f)))
    {
        __ContentDrawerShowingOutputLog = false;
        __ContentDrawerOpen = true;
        __ContentDrawerCloseTime = ImGui::GetTime() + 0.35;
    }
    ImGui::SameLine(0.0f, 4.0f);
    if (ImGui::Button("Output Log", ImVec2(112.0f, 21.0f)))
    {
        __ContentDrawerShowingOutputLog = true;
        __ContentDrawerOpen = true;
        __ContentDrawerCloseTime = ImGui::GetTime() + 0.35;
    }
    ImGui::SameLine(0.0f, 12.0f);
    if (ImGui::Button(__ContentDrawerPinned ? "Unpin" : "Pin", ImVec2(64.0f, 21.0f)))
    {
        __ContentDrawerPinned = false == __ContentDrawerPinned;
        __ContentDrawerOpen = true;
        __ContentDrawerCloseTime = ImGui::GetTime() + 0.35;
    }
    ImGui::SameLine(0.0f, 4.0f);
    if (ImGui::Button(__ContentDrawerOpen ? "v" : "^", ImVec2(28.0f, 21.0f)))
    {
        if (__ContentDrawerPinned)
            __ContentDrawerPinned = false;
        __ContentDrawerOpen = false == __ContentDrawerOpen;
        __ContentDrawerCloseTime = ImGui::GetTime() + 0.35;
    }
    const bool StatusBarHovered = ImGui::IsMouseHoveringRect(
        StatusBarMinimum,
        StatusBarMaximum,
        false);
    ImGui::EndChild();
    ImGui::PopStyleColor();

    const double CurrentTime = ImGui::GetTime();
    if (__ContentDrawerPinned || DrawerHovered || ResizeHandleHovered || ResizeHandleActive || StatusBarHovered || ImGui::GetDragDropPayload() ||
        ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
    {
        __ContentDrawerOpen = true;
        __ContentDrawerCloseTime = CurrentTime + 0.35;
    }
    else if (__ContentDrawerOpen && CurrentTime >= __ContentDrawerCloseTime)
    {
        __ContentDrawerOpen = false;
    }
    ImGui::End();
}

void FEditorImGui::DrawGameModeSelector(const char* _Label)
{
    const char* CurrentGameMode = __UseEngineGameMode
        ? "GameModeBase" : "ProjectGameMode";
    ImGui::BeginDisabled(__IsPlaying);
    if (ImGui::BeginCombo(_Label, CurrentGameMode))
    {
        if (ImGui::Selectable("GameModeBase", __UseEngineGameMode))
            PostMessageW(__MainWindow, WM_COMMAND, IDM_DEFAULTPAWN_MANNEQUIN, 0);
        if (ImGui::Selectable("ProjectGameMode", false == __UseEngineGameMode))
            PostMessageW(__MainWindow, WM_COMMAND, IDM_DEFAULTPAWN_PROJECTPLAYER, 0);
        ImGui::EndCombo();
    }
    ImGui::EndDisabled();
}

void FEditorImGui::DrawWorldSettings()
{
    if (false == __ShowWorldSettings)
        return;

    ImGui::Begin("World Settings", &__ShowWorldSettings);
    if (ImGui::CollapsingHeader("GameMode", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const char* Modes[] = {"None (Project Default)", "GameModeBase", "ProjectGameMode"};
        int Mode = __LevelGameMode + 1;
        ImGui::BeginDisabled(__IsPlaying);
        if (ImGui::Combo("GameMode Override", &Mode, Modes, 3)) __LevelGameMode = Mode - 1;
        ImGui::EndDisabled();
        ImGui::TextDisabled("Saved with this level.");
    }
    ImGui::End();
}

void FEditorImGui::DrawProjectSettings()
{
    if (false == __ShowProjectSettings)
        return;

    ImGui::SetNextWindowSize(ImVec2(760.0f, 500.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Project Settings", &__ShowProjectSettings);
    ImGui::BeginChild("ProjectSettingsCategories", ImVec2(190.0f, 0.0f),
        ImGuiChildFlags_Borders);
    ImGui::TextDisabled("Project");
    if (ImGui::Selectable("Maps & Modes", !__ProjectSettingsInputPage && !__ProjectSettingsCollisionPage))
    { __ProjectSettingsInputPage = false; __ProjectSettingsCollisionPage = false; }
    ImGui::Spacing();
    ImGui::TextDisabled("Engine");
    if (ImGui::Selectable("Input", __ProjectSettingsInputPage && !__ProjectSettingsCollisionPage))
    { __ProjectSettingsInputPage = true; __ProjectSettingsCollisionPage = false; }
    if (ImGui::Selectable("Collision", __ProjectSettingsCollisionPage)) __ProjectSettingsCollisionPage = true;
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("ProjectSettingsDetails", ImVec2(0.0f, 0.0f));
    ImGui::TextUnformatted(__ProjectSettingsCollisionPage ? "Collision" : (__ProjectSettingsInputPage ? "Input" : "Maps & Modes"));
    ImGui::Separator();
    if (__ProjectSettingsCollisionPage) DrawCollisionSettings();
    else if (__ProjectSettingsInputPage)
    {
        static char Search[64] = {};
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputTextWithHint("##InputSearch", "Search Input Settings", Search, sizeof(Search));

        static char NewMappingName[64] = {};
        if (ImGui::Button("+ Action Mapping"))
            ImGui::OpenPopup("AddActionMapping");
        ImGui::SameLine();
        if (ImGui::Button("+ Axis Mapping"))
            ImGui::OpenPopup("AddAxisMapping");
        if (ImGui::BeginPopupModal("AddActionMapping", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::InputTextWithHint("##Name", "Action Name", NewMappingName, sizeof(NewMappingName));
            if (ImGui::Button("Add") && '\0' != NewMappingName[0])
            {
                SetProjectActionMapping(NewMappingName, EKey::SpaceBar);
                NewMappingName[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopupModal("AddAxisMapping", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::InputTextWithHint("##Name", "Axis Name", NewMappingName, sizeof(NewMappingName));
            if (ImGui::Button("Add") && '\0' != NewMappingName[0])
            {
                SetProjectAxisMapping(NewMappingName, EKey::SpaceBar, 1.0f);
                NewMappingName[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        auto DrawKeySelector = [](const char* _ID, EKey _CurrentKey, bool _IsWaiting, auto _OnClicked)
        {
            const auto Label = _IsWaiting ? FString(TEXT("Press a key...")) :
                (EKey::Invalid == _CurrentKey ? FString(TEXT("Select a key")) : GetKeyName(_CurrentKey));
            if (ImGui::Button(Label.ToUtf8().c_str(), ImVec2(220.0f, 0.0f)))
                _OnClicked();
        };

        if (ImGui::CollapsingHeader("Action Mappings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            std::map<FString, std::vector<FInputActionKeyMapping>> Groups;
            for (const auto& Mapping : GetProjectActionMappings())
                Groups[Mapping.__ActionName].push_back(Mapping);
            for (auto& [Name, Mappings] : Groups)
            {
                ImGui::PushID(Name.ToUtf8().c_str());
                if (ImGui::TreeNodeEx(Name.ToUtf8().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                {
                    for (size_t Index = 0; Index < Mappings.size(); ++Index)
                    {
                        const auto Mapping = Mappings[Index];
                        ImGui::PushID(static_cast<int>(Index));
                        DrawKeySelector("##Key", Mapping.__Key, IsWaitingForAction(Mapping), [this, Mapping]
                        {
                            __ActionKeyToReplace = Mapping;
                            __WaitingForKey = true;
                        });
                        ImGui::SameLine();
                        if (ImGui::SmallButton("X"))
                            RemoveProjectActionMapping(Mapping);
                        ImGui::PopID();
                    }
                    if (ImGui::SmallButton("+ Add Key"))
                        SetProjectActionMapping(Name, EKey::Invalid);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        }

        if (ImGui::CollapsingHeader("Axis Mappings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            std::map<FString, std::vector<FInputAxisKeyMapping>> Groups;
            for (const auto& Mapping : GetProjectAxisMappings())
                Groups[Mapping.__AxisName].push_back(Mapping);
            for (auto& [Name, Mappings] : Groups)
            {
                ImGui::PushID(Name.ToUtf8().c_str());
                if (ImGui::TreeNodeEx(Name.ToUtf8().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                {
                    for (size_t Index = 0; Index < Mappings.size(); ++Index)
                    {
                        const auto Mapping = Mappings[Index];
                        ImGui::PushID(static_cast<int>(Index));
                        DrawKeySelector("##Key", Mapping.__Key, IsWaitingForAxis(Mapping), [this, Mapping]
                        {
                            __AxisKeyToReplace = Mapping;
                            __WaitingForAxis = true;
                        });
                        ImGui::SameLine();
                        float Scale = Mapping.__Scale;
                        ImGui::SetNextItemWidth(110.0f);
                        if (ImGui::DragFloat("Scale", &Scale, 0.05f))
                        {
                            RemoveProjectAxisMapping(Mapping);
                            SetProjectAxisMapping(Mapping.__AxisName, Mapping.__Key, Scale);
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("X"))
                            RemoveProjectAxisMapping(Mapping);
                        ImGui::PopID();
                    }
                    if (ImGui::SmallButton("+ Add Key"))
                        SetProjectAxisMapping(Name, EKey::Invalid, 1.0f);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        }
    }
    else if (ImGui::CollapsingHeader("Default Modes", ImGuiTreeNodeFlags_DefaultOpen))
    {
        DrawMapSettings();
    }
    ImGui::EndChild();
    ImGui::End();
}

bool FEditorImGui::DrawContentBrowser(float _Height)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.102f, 0.102f, 0.102f, 1.0f));
    ImGui::BeginChild(
        "ContentBrowserDrawer",
        ImVec2(0.0f, _Height),
        ImGuiChildFlags_Borders,
        ImGuiWindowFlags_NoSavedSettings);
    const ImVec2 DrawerMinimum = ImGui::GetWindowPos();
    const ImVec2 DrawerSize = ImGui::GetWindowSize();
    const ImVec2 DrawerMaximum(
        DrawerMinimum.x + DrawerSize.x,
        DrawerMinimum.y + DrawerSize.y);
    const bool DrawerHovered = ImGui::IsMouseHoveringRect(
        DrawerMinimum,
        DrawerMaximum,
        false);
    if (false == __ContentDrawerShowingOutputLog)
    {
    const auto* ProjectPaths = GEngine ? GEngine->GetEngineSystem<PathEngineSystem>() : nullptr;
    std::filesystem::path Root = ProjectPaths
        ? ProjectPaths->GetProjectContentDirectory() : __RootDirectory;
    std::error_code Error;

    const float AvailableWidth = (std::max)(1.0f, ImGui::GetContentRegionAvail().x);
    const float SplitterWidth = (std::min)(6.0f, AvailableWidth * 0.05f);
    const float MinimumSourcesWidth = (std::min)(120.0f, AvailableWidth * 0.3f);
    const float MaximumSourcesWidth = (std::max)(MinimumSourcesWidth, AvailableWidth - SplitterWidth - (std::min)(180.0f, AvailableWidth * 0.4f));
    const float SourcesWidth = (std::clamp)(__ContentBrowserSourcesWidth, MinimumSourcesWidth, MaximumSourcesWidth);
    ImGui::BeginChild("ContentBrowserFolders", ImVec2(SourcesWidth, 0.0f), ImGuiChildFlags_Borders);
    if (!Root.empty() && std::filesystem::exists(Root, Error))
    {
        if (ImGui::Selectable("Content", __ContentSource == EContentSource::ProjectContent && (__ContentBrowserSelection.empty() || __ContentBrowserSelection == Root)))
        {
            __ContentSource = EContentSource::ProjectContent;
            __ContentBrowserSelection = Root;
            __ContentBrowserFocusedPath = Root;
        }
        ImGui::Separator();
        if (__ContentSource == EContentSource::ProjectContent) DrawContentFolder(Root);
    }
    if (__ShowEngineContent && ProjectPaths)
    {
        const auto EngineRoot = ProjectPaths->GetEngineContentDirectory();
        if (ImGui::Selectable("Engine Content", __ContentSource == EContentSource::EngineContent && __ContentBrowserSelection == EngineRoot))
        {
            __ContentSource = EContentSource::EngineContent;
            __ContentBrowserSelection = EngineRoot;
            __ContentBrowserFocusedPath = EngineRoot;
        }
        if (__ContentSource == EContentSource::EngineContent) DrawContentFolder(EngineRoot);
    }
    if (__ShowCppClasses)
    {
        ImGui::SeparatorText("C++ Classes");
        DrawClassFolders(false, {});
        if (__ShowEngineContent)
        {
            ImGui::SeparatorText("Engine C++ Classes");
            DrawClassFolders(true, {});
        }
    }
    if (__ContentSource == EContentSource::EngineContent && ProjectPaths) Root = ProjectPaths->GetEngineContentDirectory();
    ImGui::EndChild();
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::InvisibleButton("ContentBrowserSplitter", ImVec2(SplitterWidth, (std::max)(1.0f, ImGui::GetContentRegionAvail().y)));
    if (ImGui::IsItemHovered() || ImGui::IsItemActive())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::GetColorU32(ImGui::IsItemActive() ? ImGuiCol_SeparatorActive : ImGuiCol_SeparatorHovered));
    }
    if (ImGui::IsItemActive() && ImGui::GetIO().MouseDelta.x != 0.0f)
        __ContentBrowserSourcesWidth = (std::clamp)(SourcesWidth + ImGui::GetIO().MouseDelta.x, MinimumSourcesWidth, MaximumSourcesWidth);
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginChild("ContentBrowserAssets", ImVec2(0.0f, 0.0f));
    if (__ContentSource == EContentSource::ProjectClasses || __ContentSource == EContentSource::EngineClasses)
    {
        DrawNativeClassAssets();
        ImGui::EndChild();
        ImGui::EndChild();
        ImGui::PopStyleColor();
        return DrawerHovered;
    }
    if (ImGui::IsWindowHovered() && ImGui::GetIO().KeyCtrl &&
        ImGui::GetIO().MouseWheel != 0.0f)
    {
        __ContentBrowserTileSize += ImGui::GetIO().MouseWheel * 12.0f;
        __ContentBrowserTileSize = (std::clamp)(__ContentBrowserTileSize, 84.0f, 240.0f);
    }
    const std::filesystem::path DisplayPath = __ContentBrowserSelection.empty()
        ? Root : __ContentBrowserSelection;
    ImGui::Text("%s / %s", __ContentSource == EContentSource::EngineContent ? "Engine Content" : "Content", FString(DisplayPath.filename().wstring()).ToUtf8().c_str());
    ImGui::SameLine();
    DrawMaterialCreation(DisplayPath);
    ImGui::SameLine();
    if (ImGui::Button("Import...")) BeginAssetImport(DisplayPath);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(220);
    ImGui::InputTextWithHint("##AssetSearch", "Search Assets", __AssetSearch, sizeof(__AssetSearch));
    ImGui::SameLine();
    DrawContentSourceSettings();
    __DropTargets.push_back({DrawerMinimum, DrawerMaximum, DisplayPath});
    ImGui::Separator();
    std::vector<std::filesystem::directory_entry> Entries;
    for (const auto& Entry : std::filesystem::directory_iterator(DisplayPath, Error))
    {
        if (!Error && (Entry.is_directory(Error) || (Entry.is_regular_file(Error) &&
            __AssetKinds.find(Entry.path().wstring()) != __AssetKinds.end())))
            Entries.emplace_back(Entry);
    }
    std::sort(Entries.begin(), Entries.end(), [](const auto& Left, const auto& Right)
    {
        std::error_code Error;
        const bool LeftDirectory = Left.is_directory(Error);
        const bool RightDirectory = Right.is_directory(Error);
        if (LeftDirectory != RightDirectory)
            return LeftDirectory > RightDirectory;
        return FString(Left.path().filename().wstring()).ToUtf8() < FString(Right.path().filename().wstring()).ToUtf8();
    });
    int Column = 0;
    for (const auto& Entry : Entries)
    {
        const FString FileName = FString(Entry.path().filename().wstring()).ToUtf8();
        if (__AssetSearch[0] && !FileName.Contains(FString(__AssetSearch), ESearchCase::CaseSensitive)) continue;
        ImGui::PushID(FString(Entry.path().wstring()).ToUtf8().c_str());
        const bool IsDirectory = Entry.is_directory(Error);
        const FString DisplayName = IsDirectory ? FileName : FString(Entry.path().stem().wstring()).ToUtf8();
        const bool WasFocused = __ContentBrowserFocusedPath == Entry.path();
        if (WasFocused)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.000f, 0.439f, 0.878f, 1.0f));
        std::filesystem::path IconPath = __RootDirectory / "Engine" / "Content" /
            (IsDirectory ? "Folder.svg" : "Actor.svg");
        if (!IsDirectory)
        {
            const FString Extension = Entry.path().extension().wstring();
            if (_wcsicmp(Extension.c_str(), TEXT(".umap")) == 0)
                IconPath = __RootDirectory / "Engine" / "Content" / "World.svg";
            else if (__AssetKinds.at(Entry.path().wstring()) == 1)
                IconPath = __RootDirectory / "Engine" / "Content" / "Texture2D.svg";
        }
        ImGui::BeginGroup();
        ID3D11ShaderResourceView* Thumbnail = !IsDirectory && !_wcsicmp(Entry.path().extension().c_str(), L".uasset")
            ? GetAssetThumbnail(Entry.path()) : nullptr;
        if (ID3D11ShaderResourceView* Icon = Thumbnail ? Thumbnail : LoadIconTexture(IconPath))
        {
            const float CursorX = ImGui::GetCursorPosX();
            ImGui::SetCursorPosX(CursorX + 38.0f);
            const float IconSize = __ContentBrowserTileSize * 0.46f;
            ImGui::ImageButton("##AssetThumbnail", reinterpret_cast<ImTextureID>(Icon), ImVec2(IconSize, IconSize));
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                __ContentBrowserFocusedPath = Entry.path();
        }
        ImGui::Button(DisplayName.ToUtf8().c_str(), ImVec2(__ContentBrowserTileSize, 28.0f));
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            __ContentBrowserFocusedPath = Entry.path();
        if (!IsDirectory)
        {
            const auto Kind = __AssetKinds.find(Entry.path().wstring());
            ImGui::TextDisabled("%s", Kind != __AssetKinds.end() && Kind->second != 3 ? (Kind->second == 1 ? "Texture2D" : Kind->second == 4 ? "Material" : Kind->second == 5 ? "Material Instance" : "SoundWave") : "Level");
        }
        if (WasFocused)
            ImGui::PopStyleColor();
        ImGui::EndGroup();
        if (!IsDirectory && !_wcsicmp(Entry.path().extension().c_str(), L".uasset"))
        {
            DrawAssetDragSource(Entry.path());
            DrawAssetContextMenu(Entry.path());
        }
        if (IsDirectory) __DropTargets.push_back({ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), Entry.path()});
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            if (IsDirectory)
            {
                __ContentBrowserSelection = Entry.path();
                __ContentBrowserFocusedPath = Entry.path();
                RevealContentPath(Entry.path());
            }
            else
            {
                const FString Extension = Entry.path().extension().wstring();
                if (_wcsicmp(Extension.c_str(), TEXT(".uasset")) == 0)
                {
                    OpenAsset(Entry.path());
                }
                else if (_wcsicmp(Extension.c_str(), TEXT(".umap")) == 0)
                {
                    OpenEditorMap(Entry.path());
                }
            }
        }
        const int Columns = (std::max)(1, static_cast<int>(
            ImGui::GetContentRegionAvail().x / (__ContentBrowserTileSize + ImGui::GetStyle().ItemSpacing.x)));
        if (++Column < Columns)
            ImGui::SameLine();
        else
            Column = 0;
        ImGui::PopID();
    }
    if (Entries.empty())
        ImGui::TextDisabled("Select a folder to view its assets.");
    ImGui::EndChild();
    }
    else
    {
        ImGui::TextColored(ImVec4(0.45f, 0.75f, 1.0f, 1.0f), "LogInit: UnrealEngine Editor initialized");
        ImGui::TextUnformatted("LogRHI: DirectX 11 viewport active");
        ImGui::TextDisabled("LogEditor: Dear ImGui Win32 + DX11 backends active");
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    return DrawerHovered;
}

void FEditorImGui::DrawViewportOptions()
{
    if (ImGui::Button("v##ViewportOptions")) ImGui::OpenPopup("Viewport Options");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Viewport Options");
    if (ImGui::BeginPopup("Viewport Options"))
    {
        if (ImGui::MenuItem("Immersive Mode", "F11", __ImmersiveViewport))
            __ToggleImmersiveRequested = true;
        ImGui::MenuItem("Constrain Aspect Ratio", nullptr, &__ConstrainViewportAspectRatio);
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    ImGui::TextUnformatted("Perspective");
    if (!__EditedMap.empty())
    {
        ImGui::SameLine();
        ImGui::TextDisabled("| %s", FString(__EditedMap.stem().wstring()).ToUtf8().c_str());
    }
    if (__ImmersiveViewport)
    {
        ImGui::SameLine();
        if (ImGui::Button("Restore Viewport (F11)")) __ToggleImmersiveRequested = true;
    }
}

void FEditorImGui::DrawViewport()
{
    if (__ImmersiveViewport)
    {
        const auto* Viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(Viewport->Pos);
        ImGui::SetNextWindowSize(Viewport->Size);
    }
    ImGui::Begin(__ImmersiveViewport ? "Immersive Viewport" : "Viewport", nullptr,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        (__ImmersiveViewport ? ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove : 0));
    DrawViewportOptions();
    const ImVec2 ContentPosition = ImGui::GetCursorScreenPos();
    const ImVec2 ContentSize = ImGui::GetContentRegionAvail();

    // Keep the initial editor viewport aspect as the design aspect. The
    // native child and the D3D11 render target are both fitted to this
    // rectangle, so there is only one visible game-render area.
    if (__ViewportDesignSize.x <= 0.0f || __ViewportDesignSize.y <= 0.0f)
        __ViewportDesignSize = ContentSize;

    const ImVec2 FittedSize = __ConstrainViewportAspectRatio
        ? [&]()
          {
              const float Scale = (std::min)(
                  ContentSize.x / __ViewportDesignSize.x,
                  ContentSize.y / __ViewportDesignSize.y);
              return ImVec2(__ViewportDesignSize.x * Scale,
                            __ViewportDesignSize.y * Scale);
          }()
        : ContentSize;
    POINT Position =
    {
        static_cast<LONG>(ContentPosition.x + (ContentSize.x - FittedSize.x) * 0.5f),
        static_cast<LONG>(ContentPosition.y + (ContentSize.y - FittedSize.y) * 0.5f)
    };
    // In the single-host-window setup, ImGui screen coordinates are
    // relative to the host's client area.  Converting them with
    // ScreenToClient treats them as desktop coordinates and causes the
    // native viewport to remain at its old screen position after the
    // editor window moves.

    const int Width = static_cast<int>(FittedSize.x);
    // The native child must cover exactly the ImGui content region.  Do
    // not apply a title/tab-bar compensation here: ContentPosition and
    // ContentSize already describe the drawable Viewport area.
    const int Height = static_cast<int>(FittedSize.y);
    if (Width > 0 && Height > 0)
    {
        RECT CurrentRect = {};
        GetWindowRect(__ViewportWindow, &CurrentRect);
        POINT CurrentPosition = { CurrentRect.left, CurrentRect.top };
        ScreenToClient(__MainWindow, &CurrentPosition);
        const int CurrentWidth = CurrentRect.right - CurrentRect.left;
        const int CurrentHeight = CurrentRect.bottom - CurrentRect.top;
        if (CurrentPosition.x != Position.x || CurrentPosition.y != Position.y ||
            CurrentWidth != Width || CurrentHeight != Height)
        {
            SetWindowPos(
                __ViewportWindow,
                HWND_TOP,
                Position.x,
                Position.y,
                Width,
                Height,
                SWP_NOACTIVATE | SWP_SHOWWINDOW);
        }
    }
    if (Width > 0 && Height > 0)
    {
        const int ClipHeight = __ContentDrawerTop >= 0.0f
            ? (std::clamp)(static_cast<int>(__ContentDrawerTop) - static_cast<int>(Position.y), 0, Height)
            : Height;
        if (__ViewportClipHeight != ClipHeight || __ViewportClipWidth != Width)
        {
            wil::unique_hrgn Region(CreateRectRgn(0, 0, Width, ClipHeight));
            if (Region)
            {
                if (SetWindowRgn(__ViewportWindow, Region.get(), TRUE))
                {
                    Region.release();
                    __ViewportClipHeight = ClipHeight;
                    __ViewportClipWidth = Width;
                }
            }
        }
    }
    ImGui::Dummy(ContentSize);
    DrawViewportAssetDrop(ContentPosition, FittedSize);
    const bool HideViewport = __ShowImport || __ShowAsset || __ShowAssetPreferences || __ShowProjectSettings ||
        ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
    if (IsWindowVisible(__ViewportWindow) == HideViewport)
        ShowWindow(__ViewportWindow, HideViewport ? SW_HIDE : SW_SHOWNA);
    ImGui::End();
}


void FEditorImGui::InitializeLayoutSettings()
{
    if (!__LayoutIniFilename.IsEmpty()) return;
    auto* Paths = GEngine ? GEngine->GetEngineSystem<PathEngineSystem>() : nullptr;
    if (!Paths) return;
    const auto File = Paths->GetProjectSavedDirectory() / L"Config/EditorLayout.ini";
    std::error_code Error;
    std::filesystem::create_directories(File.parent_path(), Error);
    if (Error) return;
    __LayoutIniFilename = FString(File.wstring());
    const auto Utf8 = __LayoutIniFilename.ToUtf8();
    __LayoutIniUtf8.assign(Utf8.begin(), Utf8.end());
    __LayoutIniUtf8.push_back(0);
    ImGuiSettingsHandler Handler;
    Handler.TypeName = "UnrealEngineLayout";
    Handler.TypeHash = ImHashStr(Handler.TypeName);
    Handler.UserData = this;
    Handler.ReadOpenFn = [](ImGuiContext*, ImGuiSettingsHandler* _Handler, const char* _Name) -> void*
    {
        return std::strcmp(_Name, "Editor") == 0 ? _Handler->UserData : nullptr;
    };
    Handler.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler*, void* _Entry, const char* _Line)
    {
        auto& Editor = *static_cast<FEditorImGui*>(_Entry);
        float Value = 0.0f;
        int Flag = 0;
        if (std::sscanf(_Line, "SourcesWidth=%f", &Value) == 1 && std::isfinite(Value))
            Editor.__ContentBrowserSourcesWidth = (std::clamp)(Value, 120.0f, 4096.0f);
        else if (std::sscanf(_Line, "TileSize=%f", &Value) == 1 && std::isfinite(Value))
            Editor.__ContentBrowserTileSize = (std::clamp)(Value, 84.0f, 240.0f);
        else if (std::sscanf(_Line, "DrawerHeight=%f", &Value) == 1 && std::isfinite(Value))
            Editor.__ContentDrawerHeight = (std::clamp)(Value, 100.0f, 4096.0f);
        else if (std::sscanf(_Line, "DrawerPinned=%d", &Flag) == 1)
            Editor.__ContentDrawerPinned = Flag != 0;
        else if (std::sscanf(_Line, "WorldSettings=%d", &Flag) == 1)
            Editor.__ShowWorldSettings = Flag != 0;
        else if (std::sscanf(_Line, "ProjectSettings=%d", &Flag) == 1)
            Editor.__ShowProjectSettings = Flag != 0;
    };
    Handler.WriteAllFn = [](ImGuiContext*, ImGuiSettingsHandler* _Handler, ImGuiTextBuffer* _Output)
    {
        const auto& Editor = *static_cast<FEditorImGui*>(_Handler->UserData);
        _Output->appendf("[UnrealEngineLayout][Editor]\nSourcesWidth=%.3f\nTileSize=%.3f\nDrawerHeight=%.3f\nDrawerPinned=%d\nWorldSettings=%d\nProjectSettings=%d\n\n",
            Editor.__ContentBrowserSourcesWidth, Editor.__ContentBrowserTileSize, Editor.__ContentDrawerHeight,
            Editor.__ContentDrawerPinned, Editor.__ShowWorldSettings, Editor.__ShowProjectSettings);
    };
    ImGui::AddSettingsHandler(&Handler);
    ImGui::GetIO().IniFilename = __LayoutIniUtf8.data();
    ImGui::LoadIniSettingsFromDisk(__LayoutIniUtf8.data());
    __DockLayoutCreated = false;
    __ContentDrawerOpen = __ContentDrawerPinned;
    __ContentDrawerOpenAmount = __ContentDrawerPinned ? 1.0f : 0.0f;
}

bool FEditorImGui::Initialize(HWND _MainWindow, HWND _ViewportWindow)
{
    if (nullptr == _MainWindow || nullptr == _ViewportWindow)
        return false;

    __MainWindow = _MainWindow;
    __ViewportWindow = _ViewportWindow;
    std::vector<wchar_t> Executable(32768, 0);
    const DWORD Length = GetModuleFileNameW(nullptr, Executable.data(),
        static_cast<DWORD>(Executable.size()));
    if (0 == Length || Length >= Executable.size())
        return false;
    Executable[Length] = 0;
    __RootDirectory = std::filesystem::path(Executable.data()).parent_path();
    std::error_code Error;
    while (!std::filesystem::is_directory(__RootDirectory / "Engine" / "Content", Error))
    {
        const std::filesystem::path Parent = __RootDirectory.parent_path();
        if (Parent == __RootDirectory || Parent.empty())
            return false;
        __RootDirectory = Parent;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& IO = ImGui::GetIO();
    IO.IniFilename = nullptr;
    IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    if (std::filesystem::exists("C:/Windows/Fonts/malgun.ttf"))
        IO.FontDefault = IO.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/malgun.ttf", 14.0f, nullptr, IO.Fonts->GetGlyphRangesKorean());
    else if (std::filesystem::exists("C:/Windows/Fonts/segoeui.ttf"))
        IO.FontDefault = IO.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", 14.0f);
    ApplyEditorStyle();

    if (false == CreateD3D11Resources(_MainWindow) ||
        false == ImGui_ImplWin32_Init(_MainWindow) ||
        false == ImGui_ImplDX11_Init(__Device.Get(), __DeviceContext.Get()))
    {
        Shutdown();
        return false;
    }
    return true;
}

void FEditorImGui::Shutdown()
{
    __CollisionPreviewWorld.reset();
    __AutoReimport.reset();
    __EditorActorSubsystem.reset();
    __AssetThumbnails.clear();
    __PreviewAudio.reset();
    __PreviewTexture.Reset();
    __PreviewAsset = nullptr;
    __IconTextures.clear();
    if (nullptr == ImGui::GetCurrentContext())
        return;

    if (!__LayoutIniFilename.IsEmpty()) ImGui::SaveIniSettingsToDisk(__LayoutIniUtf8.data());
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    __RenderTargetView.Reset();
    __SwapChain.Reset();
    __DeviceContext.Reset();
    __Device.Reset();
    __MainWindow = nullptr;
    __ViewportWindow = nullptr;
    __DockLayoutCreated = false;
    __LayoutIniFilename.Empty();
    __LayoutIniUtf8.clear();
}

void FEditorImGui::Render()
{
    if (nullptr == ImGui::GetCurrentContext() || __IsRenderingEditorImGui)
        return;

    __IsRenderingEditorImGui = true;
    using FProfileClock = std::chrono::steady_clock;
    static std::ofstream Profile = []
    {
        wchar_t Path[32768] = {};
        const auto Length = GetEnvironmentVariableW(L"UE_EDITOR_PROFILE", Path, 32768);
        std::ofstream Output;
        if (Length && Length < 32768)
        {
            Output.open(std::filesystem::path(Path));
            Output << "playing,assets_ms,panels_ms,viewport_ms,present_ms,total_ms\n";
        }
        return Output;
    }();
    const auto ProfileStart = FProfileClock::now();
    InitializeLayoutSettings();
    const auto LayoutBefore = std::make_tuple(__ContentBrowserSourcesWidth, __ContentBrowserTileSize, __ContentDrawerHeight, __ContentDrawerPinned, __ShowWorldSettings, __ShowProjectSettings);
    UpdateContentSources();
    TickAssetWorkflow();
    TickProjectSession();
    __DropTargets.clear();
    const auto ProfileAssets = FProfileClock::now();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    if (__ToggleImmersiveRequested)
    {
        __ImmersiveViewport = !__ImmersiveViewport;
        __ToggleImmersiveRequested = false;
    }
    if (__ImmersiveViewport)
    {
        __ContentDrawerTop = -1.0f;
        if (auto* Host = ImGui::FindWindowByName("UnrealEngine Editor"))
            ImGui::DockSpace(Host->GetID("UnrealEngineDockspace"), ImVec2(0, 0), ImGuiDockNodeFlags_KeepAliveOnly);
    }
    else
    {
        DrawMainDockspace();
        DrawWorldOutliner();
        DrawDetails();
        DrawWorldSettings();
        DrawProjectSettings();
        DrawCollisionPreview();
    }
    const auto ProfilePanels = FProfileClock::now();
    DrawViewport();
    DrawAssetWindows();
    DrawFileDialogs();
    if (LayoutBefore != std::make_tuple(__ContentBrowserSourcesWidth, __ContentBrowserTileSize, __ContentDrawerHeight, __ContentDrawerPinned, __ShowWorldSettings, __ShowProjectSettings))
        ImGui::MarkIniSettingsDirty();
    ImGui::Render();
    const auto ProfileViewport = FProfileClock::now();

    if (!__RenderTargetView)
    {
        __IsRenderingEditorImGui = false;
        return;
    }

    constexpr float ClearColor[4] = { 0.082f, 0.082f, 0.082f, 1.0f };
    ID3D11RenderTargetView* RenderTarget = __RenderTargetView.Get();
    __DeviceContext->OMSetRenderTargets(1, &RenderTarget, nullptr);
    __DeviceContext->ClearRenderTargetView(RenderTarget, ClearColor);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    __SwapChain->Present(1, 0);
    if (Profile.is_open())
    {
        const auto End = FProfileClock::now();
        auto Milliseconds = [](auto _Start, auto _End) { return std::chrono::duration<double, std::milli>(_End - _Start).count(); };
        Profile << __IsPlaying << ',' << Milliseconds(ProfileStart, ProfileAssets) << ','
            << Milliseconds(ProfileAssets, ProfilePanels) << ',' << Milliseconds(ProfilePanels, ProfileViewport) << ','
            << Milliseconds(ProfileViewport, End) << ',' << Milliseconds(ProfileStart, End) << '\n';
        Profile.flush();
    }
    __IsRenderingEditorImGui = false;
}

void FEditorImGui::Resize(unsigned int _Width, unsigned int _Height)
{
    if (!__SwapChain || 0 == _Width || 0 == _Height)
        return;

    __DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    __RenderTargetView.Reset();
    if (SUCCEEDED(__SwapChain->ResizeBuffers(0, _Width, _Height, DXGI_FORMAT_UNKNOWN, 0)))
        CreateRenderTarget();
}

void FEditorImGui::SetEditorState(
    bool _IsPlaying,
    bool _IsPaused,
    bool _UseEngineGameMode)
{
    if (__IsPlaying != _IsPlaying) __WaitingForWorldChange = true;
    __IsPlaying = _IsPlaying;
    __IsPaused = _IsPaused;
    __UseEngineGameMode = _UseEngineGameMode;
}

bool FEditorImGui::HandleWindowMessage(
    HWND _Window,
    unsigned int _Message,
    unsigned long long _WParam,
    long long _LParam)
{
    if (nullptr == ImGui::GetCurrentContext())
        return false;

    if (_Message == WM_KEYDOWN && _WParam == VK_F11 &&
        !__WaitingForKey && !__WaitingForAxis && !ImGui::GetIO().WantTextInput &&
        !(GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_SHIFT) & 0x8000) &&
        !(GetKeyState(VK_MENU) & 0x8000))
    {
        if (!(_LParam & (1LL << 30))) __ToggleImmersiveRequested = true;
        return true;
    }

    if (__WaitingForKey || __WaitingForAxis)
    {
        if (WM_KEYDOWN == _Message || WM_SYSKEYDOWN == _Message)
        {
            const EKey Key = InputEngineSystem::GetKeyFromVirtualKey(
                static_cast<uint32_t>(_WParam));
            if (EKey::Escape == Key)
            {
                __WaitingForKey = false;
                __WaitingForAxis = false;
                return true;
            }
            if (EKey::Invalid != Key)
            {
                if (__WaitingForKey)
                {
                    RemoveProjectActionMapping(__ActionKeyToReplace);
                    SetProjectActionMapping(__ActionKeyToReplace.__ActionName, Key);
                }
                else
                {
                    RemoveProjectAxisMapping(__AxisKeyToReplace);
                    SetProjectAxisMapping(
                        __AxisKeyToReplace.__AxisName, Key, __AxisKeyToReplace.__Scale);
                }
                __WaitingForKey = false;
                __WaitingForAxis = false;
                return true;
            }
        }
    }

    return 0 != ImGui_ImplWin32_WndProcHandler(
        _Window,
        static_cast<UINT>(_Message),
        static_cast<WPARAM>(_WParam),
        static_cast<LPARAM>(_LParam));
}

FEditorImGui::FEditorImGui() = default;

FEditorImGui::~FEditorImGui() = default;
