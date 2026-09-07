


#include "framework.h"
#include "Editor.h"
#include "EngineLibraries.h"
#include "Game/Project/Project.h"
#include "UE/DynamicRHI.h"
#include "EditorImGui.h"

#include <dwmapi.h>
#include <filesystem>
#include <fstream>

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
    constexpr wchar_t EditorViewportWindowClass[] = L"ProjectEngineEditorViewport";

    LRESULT CALLBACK EditorViewportWndProc(
        HWND _Window,
        UINT _Message,
        WPARAM _WParam,
        LPARAM _LParam)
    {
        switch (_Message)
        {
        case WM_ERASEBKGND:
            // The active RHI owns every pixel in this window.
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
    }

}


void SaveGameModeSelection(const char* _GameModeName)
{
    const std::filesystem::path RelativePath =
        std::filesystem::path("Saved") / "Config" / "EditorDefaultPawn.cfg";
    const std::filesystem::path Paths[] =
    {
        RelativePath,
        std::filesystem::path("..") / "Client" / RelativePath
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

std::string ReadGameModeSelection()
{
    std::ifstream Input(std::filesystem::path("Saved") / "Config" /
        "EditorDefaultPawn.cfg");
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

   bUseEngineGameMode = ReadGameModeSelection() == "EngineGameMode";
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

    // The embedded engine input application is attached to the child Viewport HWND.
    // Forward editor-window keyboard input while PIE is active so toolbar focus does
    // not prevent the possessed pawn from receiving movement keys.
    if (bIsPlaying && nullptr != gViewportWindow &&
        (WM_KEYDOWN == message || WM_KEYUP == message ||
            WM_SYSKEYDOWN == message || WM_SYSKEYUP == message))
    {
        PostMessageW(gViewportWindow, message, wParam, lParam);
    }

    switch (message)
    {
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
                return 0;
            }
        }
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            
            switch (wmId)
            {
            case IDM_EXIT:
                DestroyWindow(hWnd);
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
                if (!bIsPlaying && InitializeProjectWithGameMode(bUseEngineGameMode))
                {
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
            // Defer layout, child-window movement and swap-chain resizing
            // until the interactive resize loop has finished.  Updating all
            // three while WM_SIZE is streaming produces a sequence of
            // visibly different dock layouts and viewport positions.
            if (gIsInteractiveResize)
            {
                // Let ImGui redraw at the new size, but keep the native child
                // at its last committed rectangle. Expanding it to the whole
                // client area would cover the Outliner/Details/Dock UI while
                // the interactive resize loop is still in progress.
                GEditorImGui.Resize(LOWORD(lParam), HIWORD(lParam));
                GEditorImGui.Render();
            }
            else
            {
                MoveWindow(gViewportWindow, 0, 0, LOWORD(lParam), HIWORD(lParam), FALSE);
                GEditorImGui.Resize(LOWORD(lParam), HIWORD(lParam));
            }
            // WM_SIZE exposes the newly sized swap-chain immediately. Draw a
            // fresh frame on the next timer tick. Rendering synchronously from
            // WM_SIZE re-enters the Dock/Viewport layout while Windows is
            // still reporting resize messages and causes visible jitter.
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        break;
    case WM_ERASEBKGND:
        // Never expose the system COLOR_WINDOW (white) brush during a live
        // resize while the D3D back buffer is being committed.
        {
            RECT ClientRect = {};
            GetClientRect(hWnd, &ClientRect);
            HBRUSH Background = CreateSolidBrush(RGB(21, 21, 21));
            FillRect(reinterpret_cast<HDC>(wParam), &ClientRect, Background);
            DeleteObject(Background);
        }
        return 1;
    case WM_ENTERSIZEMOVE:
        gIsInteractiveResize = true;
        break;
    case WM_EXITSIZEMOVE:
        gIsInteractiveResize = false;
        // WM_SIZE has already delivered the final client dimensions. Let the
        // next regular frame settle the ImGui dock layout first, then place
        // the native viewport from that same layout exactly once.
        InvalidateRect(hWnd, nullptr, FALSE);
        break;
    case WM_DESTROY:
        KillTimer(hWnd, 1);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
