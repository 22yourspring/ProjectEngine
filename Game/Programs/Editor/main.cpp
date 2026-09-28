


#include "framework.h"
#include "Editor.h"
#include "EngineLibraries.h"
#include "GameModuleAccess.h"
#include "UE/PathEngineSystem.h"
#include "UE/DynamicRHI.h"
#include "UE/GameEngine.h"
#include "EditorImGui.h"
#include "ThirdParty/ImGui/backends/imgui_impl_win32.h"

#include <dwmapi.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <shellapi.h>
#include "ThirdParty/WIL/include/wil/resource.h"

#pragma comment(lib, "Dwmapi.lib")

#define MAX_LOADSTRING 100


WCHAR szTitle[MAX_LOADSTRING];                  
WCHAR szWindowClass[MAX_LOADSTRING];            
bool bIsPlaying = false;
bool bIsPaused = false;
bool bUseEngineGameMode = true;
HWND gEditorWindow = nullptr;
HWND gViewportWindow = nullptr;
bool gIsInteractiveResize = false;
FEditorImGui GEditorImGui;


ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

namespace
{
    constexpr wchar_t EditorViewportWindowClass[] = TEXT("UnrealEngineEditorViewport");

    LRESULT CALLBACK EditorViewportWndProc(
        HWND _Window,
        UINT _Message,
        WPARAM _WParam,
        LPARAM _LParam)
    {
        switch (_Message)
        {
        case WM_KEYDOWN:
            if (_WParam == VK_F11 && GEditorImGui.HandleWindowMessage(_Window, _Message, _WParam, _LParam))
                return 0;
            break;
        case WM_DROPFILES:
            GEditorImGui.HandleFileDrop(reinterpret_cast<void*>(_WParam));
            return 0;
        case WM_ERASEBKGND:
            return TRUE;
        case WM_PAINT:
            {
                PAINTSTRUCT Paint = {};
                BeginPaint(_Window, &Paint);
                EndPaint(_Window, &Paint);
            }
            return 0;
        default:
            return DefWindowProcW(_Window, _Message, _WParam, _LParam);
        }
        return DefWindowProcW(_Window, _Message, _WParam, _LParam);
    }

}


void SaveGameModeSelection(const char* _GameModeName)
{
    if (!GEngine)
        return;
    const auto* ProjectPaths = GEngine->GetEngineSystem<PathEngineSystem>();
    if (!ProjectPaths)
        return;
    const std::filesystem::path Paths[] =
    {
        ProjectPaths->GetProjectSavedDirectory() / TEXT("Config") / TEXT("EditorDefaultPawn.cfg")
    };

    for (const std::filesystem::path& Path : Paths)
    {
        std::error_code Error;
        std::filesystem::create_directories(Path.parent_path(), Error);
        if (Error)
            continue;

        std::ofstream Output(Path, std::ios::trunc);
        if (Output.is_open())
            Output << _GameModeName << '\n';
    }
}

FString ReadGameModeSelection()
{
    if (!GEngine)
        return {};
    const auto* ProjectPaths = GEngine->GetEngineSystem<PathEngineSystem>();
    if (!ProjectPaths)
        return {};
    std::ifstream Input(ProjectPaths->GetProjectSavedDirectory() / TEXT("Config") /
        TEXT("EditorDefaultPawn.cfg"));
    std::string Selection;
    if (Input.is_open())
        Input >> Selection;
    return Selection;
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    ImGui_ImplWin32_EnableDpiAwareness();

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    

    
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_EDITOR, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    RHISetPreferredInterface(ERHIInterfaceType::D3D11);
    if (false == GEditorImGui.Initialize(gEditorWindow, gViewportWindow))
        return FALSE;

    const LONG_PTR BorderlessWindowStyle =
        (GetWindowLongPtrW(gEditorWindow, GWL_STYLE) & ~WS_CAPTION) |
        WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
    SetWindowLongPtrW(gEditorWindow, GWL_STYLE, BorderlessWindowStyle);
    SetWindowPos(
        gEditorWindow,
        nullptr,
        0,
        0,
        0,
        0,
        SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    GEditorImGui.SetEditorState(bIsPlaying, bIsPaused, bUseEngineGameMode);
    GEditorImGui.Render();
    SetTimer(gEditorWindow, 1, 16, nullptr);

    MSG msg;

    FModuleManager ModuleManager;

    TRAIIPattern_ThreadGuard EngineLoopGuard
    (
        []
        {
            RECT ViewportRect = {};
            GetClientRect(gViewportWindow, &ViewportRect);
            return FEngineLoop::GetInstance()->InitializeForEmbeddedViewport(
                gViewportWindow,
                { 
                    ViewportRect.right - ViewportRect.left,
                    ViewportRect.bottom - ViewportRect.top });
                },
        []
        {
            FEngineLoop::GetInstance()->Deinitialize();
        }
    );

    if (EngineLoopGuard.Failed())
    {
        KillTimer(gEditorWindow, 1);
        GEditorImGui.Shutdown();
        return FALSE;
    }

    
    PathEngineSystem* Paths = GEngine->GetEngineSystem<PathEngineSystem>();
    if (!Paths || !ModuleManager.LoadGameModule(Paths->GetProjectModuleFile(TEXT("Editor"))))
    {
        KillTimer(gEditorWindow, 1);
        GEditorImGui.Shutdown();
        MessageBoxW(nullptr, TEXT("Failed to load the selected project's game module."),
            TEXT("UnrealEngine"), MB_OK | MB_ICONERROR);
        return FALSE;
    }
    LoadProjectInputMappings();

    bUseEngineGameMode = ReadGameModeSelection() == "EngineGameMode";
    GEditorImGui.SetEditorState(bIsPlaying, bIsPaused, bUseEngineGameMode);
    GEditorImGui.InitializeProjectSession();

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    KillTimer(gEditorWindow, 1);
    GEditorImGui.Shutdown();
    return (int) msg.wParam;
}








ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EDITOR));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = nullptr;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    const ATOM EditorWindowClassAtom = RegisterClassExW(&wcex);
    if (0 == EditorWindowClassAtom)
        return 0;

    WNDCLASSEXW ViewportClass = {};
    ViewportClass.cbSize = sizeof(WNDCLASSEXW);
    ViewportClass.style = CS_OWNDC;
    ViewportClass.lpfnWndProc = EditorViewportWndProc;
    ViewportClass.hInstance = hInstance;
    ViewportClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    ViewportClass.hbrBackground = nullptr;
    ViewportClass.lpszClassName = EditorViewportWindowClass;

    if (0 == RegisterClassExW(&ViewportClass))
        return 0;

    return EditorWindowClassAtom;
}











BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd = CreateWindowExW(
      0,
      szWindowClass,
      szTitle,
      (WS_OVERLAPPEDWINDOW & ~WS_CAPTION) | WS_CLIPCHILDREN,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   const BOOL UseDarkTitleBar = TRUE;
   DwmSetWindowAttribute(
       hWnd,
       DWMWA_USE_IMMERSIVE_DARK_MODE,
       &UseDarkTitleBar,
       sizeof(UseDarkTitleBar));

   gEditorWindow = hWnd;
   RECT EditorClientRect = {};
   GetClientRect(hWnd, &EditorClientRect);
   gViewportWindow = CreateWindowExW(
       0,
       EditorViewportWindowClass,
       nullptr,
       WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
       0,
       0,
       EditorClientRect.right - EditorClientRect.left,
       EditorClientRect.bottom - EditorClientRect.top,
       hWnd,
       nullptr,
       hInstance,
       nullptr);

   if (nullptr == gViewportWindow)
       return FALSE;

   DragAcceptFiles(hWnd, TRUE);
   DragAcceptFiles(gViewportWindow, TRUE);

   ShowWindow(hWnd, SW_MAXIMIZE);
   UpdateWindow(hWnd);

   return TRUE;
}











LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (bIsPlaying && WM_KEYDOWN == message && VK_ESCAPE == wParam)
    {
        PostMessageW(hWnd, WM_COMMAND, IDM_STOP, 0);
        return TRUE;
    }

    if (GEditorImGui.HandleWindowMessage(
        hWnd,
        message,
        static_cast<unsigned long long>(wParam),
        static_cast<long long>(lParam)))
    {
        return TRUE;
    }

    if (bIsPlaying && nullptr != gViewportWindow &&
        (WM_KEYDOWN == message || WM_KEYUP == message ||
            WM_SYSKEYDOWN == message || WM_SYSKEYUP == message))
    {
        PostMessageW(gViewportWindow, message, wParam, lParam);
    }

    switch (message)
    {
    case WM_DROPFILES:
        GEditorImGui.HandleFileDrop(reinterpret_cast<void*>(wParam));
        return 0;
    case WM_GETMINMAXINFO:
        {
            MONITORINFO MonitorInfo = {};
            MonitorInfo.cbSize = sizeof(MonitorInfo);
            const HMONITOR Monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
            if (GetMonitorInfoW(Monitor, &MonitorInfo))
            {
                MINMAXINFO* MinMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
                const RECT& WorkArea = MonitorInfo.rcWork;
                const RECT& MonitorArea = MonitorInfo.rcMonitor;
                MinMaxInfo->ptMaxPosition.x = WorkArea.left - MonitorArea.left;
                MinMaxInfo->ptMaxPosition.y = WorkArea.top - MonitorArea.top;
                MinMaxInfo->ptMaxSize.x = WorkArea.right - WorkArea.left;
                MinMaxInfo->ptMaxSize.y = WorkArea.bottom - WorkArea.top;
                MinMaxInfo->ptMaxTrackSize = MinMaxInfo->ptMaxSize;
                return 0;
            }
        }
        break;
    case WM_NCCALCSIZE:
        if (wParam && IsZoomed(hWnd))
            return 0;
        return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_DPICHANGED:
        {
            const RECT& SuggestedRect = *reinterpret_cast<const RECT*>(lParam);
            SetWindowPos(
                hWnd, nullptr,
                SuggestedRect.left, SuggestedRect.top,
                SuggestedRect.right - SuggestedRect.left,
                SuggestedRect.bottom - SuggestedRect.top,
                SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return 0;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            
            switch (wmId)
            {
            case IDM_EXIT:
                GEditorImGui.RequestClose();
                break;
            case IDM_DEFAULTPAWN_MANNEQUIN:
                if (!bIsPlaying)
                {
                    SaveGameModeSelection("EngineGameMode");
                    bUseEngineGameMode = true;
                    GEditorImGui.SetEditorState(bIsPlaying, bIsPaused, bUseEngineGameMode);
                }
                break;
            case IDM_DEFAULTPAWN_PROJECTPLAYER:
                if (!bIsPlaying)
                {
                    SaveGameModeSelection("ProjectGameMode");
                    bUseEngineGameMode = false;
                    GEditorImGui.SetEditorState(bIsPlaying, bIsPaused, bUseEngineGameMode);
                }
                break;
            case IDM_PLAY:
                bUseEngineGameMode = GEditorImGui.UseEngineGameMode();
                if (!bIsPlaying)
                {
                    bool Started = false;
                    if (auto* EditorEngine = dynamic_cast<UGameEngine*>(GEngine))
                        EditorEngine->WithWorld([&](UWorld*) { if (InitializeProjectWithGameMode(bUseEngineGameMode)) Started = EditorEngine->PlayEditorMap(); });
                    if (!Started) break;
                    bIsPlaying = true;
                    bIsPaused = false;
                    GEditorImGui.SetEditorState(bIsPlaying, bIsPaused, bUseEngineGameMode);
                }
                break;
            case IDM_PAUSE:
                if (bIsPlaying && SetProjectPaused(false == bIsPaused))
                {
                    bIsPaused = false == bIsPaused;
                    GEditorImGui.SetEditorState(bIsPlaying, bIsPaused, bUseEngineGameMode);
                }
                break;
            case IDM_STOP:
                if (bIsPlaying && StopProject())
                {
                    bIsPlaying = false;
                    bIsPaused = false;
                    GEditorImGui.SetEditorState(bIsPlaying, bIsPaused, bUseEngineGameMode);
                }
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
            GEditorImGui.Render();
        }
        break;
    case WM_TIMER:
        if (1 == wParam)
            GEditorImGui.Render();
        break;
    case WM_SIZE:
        if (nullptr != gViewportWindow && SIZE_MINIMIZED != wParam)
        {
            if (gIsInteractiveResize)
            {
                GEditorImGui.Resize(LOWORD(lParam), HIWORD(lParam));
                GEditorImGui.Render();
            }
            else
            {
                MoveWindow(gViewportWindow, 0, 0, LOWORD(lParam), HIWORD(lParam), FALSE);
                GEditorImGui.Resize(LOWORD(lParam), HIWORD(lParam));
            }
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        break;
    case WM_ERASEBKGND:
        {
            RECT ClientRect = {};
            GetClientRect(hWnd, &ClientRect);
            wil::unique_hbrush Background(CreateSolidBrush(RGB(21, 21, 21)));
            FillRect(reinterpret_cast<HDC>(wParam), &ClientRect, Background.get());
        }
        return 1;
    case WM_ENTERSIZEMOVE:
        gIsInteractiveResize = true;
        break;
    case WM_EXITSIZEMOVE:
        gIsInteractiveResize = false;
        InvalidateRect(hWnd, nullptr, FALSE);
        break;
    case WM_CLOSE:
        GEditorImGui.RequestClose();
        return 0;
    case WM_DESTROY:
        KillTimer(hWnd, 1);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
