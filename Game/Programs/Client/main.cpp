// Client.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Client.h"
#include "Game/Project/Project.h"
#include "UE/InputSubsystem.h"

#include <CommCtrl.h>
#include <map>

#pragma comment(lib, "Comctl32.lib")

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    KeyMapping(HWND, UINT, WPARAM, LPARAM);

namespace
{
    EKey CapturedMappingKey = EKey::Invalid;
    WNDPROC OriginalKeyCaptureProc = nullptr;

    LRESULT CALLBACK KeyCaptureProc(HWND _Window, UINT _Message, WPARAM _WParam, LPARAM _LParam)
    {
        if (WM_GETDLGCODE == _Message)
            return DLGC_WANTALLKEYS;

        if (WM_KEYDOWN == _Message || WM_SYSKEYDOWN == _Message)
        {
            const EKey Key = InputSubsystem::GetKeyFromVirtualKey(
                static_cast<uint32_t>(_WParam));
            if (EKey::Invalid != Key)
            {
                CapturedMappingKey = Key;
                const std::string KeyName(GetKeyName(Key));
                SetWindowTextA(_Window, KeyName.c_str());
            }
            return 0;
        }

        return CallWindowProcW(OriginalKeyCaptureProc, _Window, _Message, _WParam, _LParam);
    }

    std::string ToUtf8(const wchar_t* _Text)
    {
        if (nullptr == _Text || L'\0' == _Text[0])
            return {};

        const int Size = WideCharToMultiByte(
            CP_UTF8, 0, _Text, -1, nullptr, 0, nullptr, nullptr);
        if (Size <= 1)
            return {};

        std::string Result(static_cast<size_t>(Size), '\0');
        WideCharToMultiByte(
            CP_UTF8, 0, _Text, -1, Result.data(), Size, nullptr, nullptr);
        Result.pop_back();
        return Result;
    }

    enum class EMappingTreeNodeType
    {
        ActionGroup,
        AxisGroup,
        ActionMapping,
        AxisMapping,
        ActionKey,
        AxisKey
    };

    struct FMappingTreeNode
    {
        EMappingTreeNodeType __Type;
        std::string __MappingName;
        FInputActionKeyMapping __ActionMapping;
        FInputAxisKeyMapping __AxisMapping;
    };

    std::vector<FMappingTreeNode> MappingTreeNodes;

    std::string GetDialogMappingName(HWND _Dialog)
    {
        wchar_t Buffer[128] = {};
        GetDlgItemTextW(_Dialog, IDC_MAPPING_NAME, Buffer, static_cast<int>(std::size(Buffer)));
        return ToUtf8(Buffer);
    }

    HTREEITEM InsertMappingTreeItem(HWND _Tree, HTREEITEM _Parent,
        const std::string& _Label, FMappingTreeNode _Node)
    {
        MappingTreeNodes.push_back(std::move(_Node));
        TVINSERTSTRUCTA Insert = {};
        Insert.hParent = _Parent;
        Insert.hInsertAfter = TVI_LAST;
        Insert.item.mask = TVIF_TEXT | TVIF_PARAM;
        Insert.item.pszText = const_cast<char*>(_Label.c_str());
        Insert.item.lParam = static_cast<LPARAM>(MappingTreeNodes.size());
        return reinterpret_cast<HTREEITEM>(
            SendMessageA(_Tree, TVM_INSERTITEMA, 0, reinterpret_cast<LPARAM>(&Insert)));
    }

    void RefreshMappingTree(HWND _Dialog)
    {
        HWND Tree = GetDlgItem(_Dialog, IDC_MAPPING_LIST);
        TreeView_DeleteAllItems(Tree);
        MappingTreeNodes.clear();

        const HTREEITEM ActionRoot = InsertMappingTreeItem(Tree, TVI_ROOT,
            "Action Mappings", { EMappingTreeNodeType::ActionGroup });
        const HTREEITEM AxisRoot = InsertMappingTreeItem(Tree, TVI_ROOT,
            "Axis Mappings", { EMappingTreeNodeType::AxisGroup });

        std::map<std::string, std::vector<FInputActionKeyMapping>> Actions;
        for (const FInputActionKeyMapping& Mapping : GetProjectActionMappings())
            Actions[Mapping.__ActionName].push_back(Mapping);

        for (const auto& [Name, Mappings] : Actions)
        {
            const HTREEITEM MappingItem = InsertMappingTreeItem(Tree, ActionRoot, Name,
                { EMappingTreeNodeType::ActionMapping, Name });
            for (const FInputActionKeyMapping& Mapping : Mappings)
            {
                const std::string Label(GetKeyName(Mapping.__Key));
                InsertMappingTreeItem(Tree, MappingItem, Label,
                    { EMappingTreeNodeType::ActionKey, Name, Mapping });
            }
            TreeView_Expand(Tree, MappingItem, TVE_EXPAND);
        }

        std::map<std::string, std::vector<FInputAxisKeyMapping>> Axes;
        for (const FInputAxisKeyMapping& Mapping : GetProjectAxisMappings())
            Axes[Mapping.__AxisName].push_back(Mapping);

        for (const auto& [Name, Mappings] : Axes)
        {
            const HTREEITEM MappingItem = InsertMappingTreeItem(Tree, AxisRoot, Name,
                { EMappingTreeNodeType::AxisMapping, Name });
            for (const FInputAxisKeyMapping& Mapping : Mappings)
            {
                char Label[128] = {};
                std::snprintf(Label, std::size(Label), "%s    Scale: %g",
                    GetKeyName(Mapping.__Key).data(), Mapping.__Scale);
                InsertMappingTreeItem(Tree, MappingItem, Label,
                    { EMappingTreeNodeType::AxisKey, Name, {}, Mapping });
            }
            TreeView_Expand(Tree, MappingItem, TVE_EXPAND);
        }

        TreeView_Expand(Tree, ActionRoot, TVE_EXPAND);
        TreeView_Expand(Tree, AxisRoot, TVE_EXPAND);
    }

    const FMappingTreeNode* GetSelectedMappingTreeNode(HWND _Dialog)
    {
        HWND Tree = GetDlgItem(_Dialog, IDC_MAPPING_LIST);
        const HTREEITEM Selection = TreeView_GetSelection(Tree);
        if (nullptr == Selection)
            return nullptr;

        TVITEMW Item = {};
        Item.mask = TVIF_PARAM;
        Item.hItem = Selection;
        if (false == TreeView_GetItem(Tree, &Item) || Item.lParam <= 0)
            return nullptr;

        const size_t Index = static_cast<size_t>(Item.lParam - 1);
        return Index < MappingTreeNodes.size() ? &MappingTreeNodes[Index] : nullptr;
    }
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    // _CrtSetBreakAlloc();

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    INITCOMMONCONTROLSEX CommonControls = { sizeof(INITCOMMONCONTROLSEX), ICC_TREEVIEW_CLASSES };
    InitCommonControlsEx(&CommonControls);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CLIENT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CLIENT));

    MSG msg;

    TRAIIPattern_ThreadGuard EngineLoopGuard
    (
        []
        {
            return FEngineLoop::GetInstance()->Initialize({ 1920, 1080 });
        },
        []
        {
            FEngineLoop::GetInstance()->Deinitialize();
        }
    );

    if (EngineLoopGuard.Failed())
        return FALSE;

    if (false == InitializeProject())
        return FALSE;

    // Main message loop:
	while (true)
	{
		if (PeekMessage(&msg, nullptr, NULL, NULL, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;

			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CLIENT));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_CLIENT);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);


   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_KEY_MAPPING:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_KEY_MAPPING), hWnd, KeyMapping);
                break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
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
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK KeyMapping(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
        {
            HWND TypeCombo = GetDlgItem(hDlg, IDC_MAPPING_TYPE);
            SendMessageW(TypeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Action"));
            SendMessageW(TypeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Axis"));
            SendMessageW(TypeCombo, CB_SETCURSEL, 1, 0);

            CapturedMappingKey = EKey::Invalid;
            HWND KeyCapture = GetDlgItem(hDlg, IDC_MAPPING_KEY);
            SetWindowTextW(KeyCapture, L"Press a key...");
            OriginalKeyCaptureProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
                KeyCapture, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(KeyCaptureProc)));
            SetFocus(KeyCapture);

            SetDlgItemTextW(hDlg, IDC_MAPPING_NAME, L"MoveHorizontal");
            SetDlgItemTextW(hDlg, IDC_MAPPING_SCALE, L"1.0");
            RefreshMappingTree(hDlg);
            return FALSE;
        }

    case WM_COMMAND:
        if (IDC_MAPPING_ADD == LOWORD(wParam))
        {
            const std::string MappingName = GetDialogMappingName(hDlg);

            if (EKey::Invalid == CapturedMappingKey || MappingName.empty())
            {
                MessageBoxW(hDlg, L"Enter a mapping name and press a key.",
                    L"Key Mapping", MB_OK | MB_ICONWARNING);
                return TRUE;
            }

            const EKey Key = CapturedMappingKey;
            const LRESULT MappingType = SendDlgItemMessageW(
                hDlg, IDC_MAPPING_TYPE, CB_GETCURSEL, 0, 0);

            bool bApplied = false;
            if (0 == MappingType)
            {
                bApplied = SetProjectActionMapping(MappingName.c_str(), Key);
            }
            else
            {
                wchar_t ScaleBuffer[32] = {};
                GetDlgItemTextW(hDlg, IDC_MAPPING_SCALE,
                    ScaleBuffer, static_cast<int>(std::size(ScaleBuffer)));
                wchar_t* ScaleEnd = nullptr;
                const float Scale = std::wcstof(ScaleBuffer, &ScaleEnd);
                if (ScaleEnd == ScaleBuffer)
                {
                    MessageBoxW(hDlg, L"Axis Scale must be a number.",
                        L"Key Mapping", MB_OK | MB_ICONWARNING);
                    return TRUE;
                }
                bApplied = SetProjectAxisMapping(MappingName.c_str(), Key, Scale);
            }

            if (false == bApplied)
            {
                MessageBoxW(hDlg, L"The project input system is not ready.",
                    L"Key Mapping", MB_OK | MB_ICONERROR);
                return TRUE;
            }

            CapturedMappingKey = EKey::Invalid;
            SetDlgItemTextW(hDlg, IDC_MAPPING_KEY, L"Press a key...");
            RefreshMappingTree(hDlg);
            return TRUE;
        }

        if (IDC_MAPPING_REMOVE == LOWORD(wParam))
        {
            const FMappingTreeNode* Node = GetSelectedMappingTreeNode(hDlg);
            if (nullptr == Node || EMappingTreeNodeType::ActionGroup == Node->__Type ||
                EMappingTreeNodeType::AxisGroup == Node->__Type)
            {
                MessageBoxW(hDlg, L"Select a mapping or key to remove.",
                    L"Key Mapping", MB_OK | MB_ICONWARNING);
                return TRUE;
            }

            bool bRemoved = false;
            if (EMappingTreeNodeType::ActionKey == Node->__Type)
                bRemoved = RemoveProjectActionMapping(Node->__ActionMapping);
            else if (EMappingTreeNodeType::AxisKey == Node->__Type)
                bRemoved = RemoveProjectAxisMapping(Node->__AxisMapping);
            else if (EMappingTreeNodeType::ActionMapping == Node->__Type)
            {
                bRemoved = true;
                for (const FInputActionKeyMapping& Mapping : GetProjectActionMappings())
                {
                    if (Mapping.__ActionName == Node->__MappingName)
                        bRemoved = RemoveProjectActionMapping(Mapping) && bRemoved;
                }
            }
            else if (EMappingTreeNodeType::AxisMapping == Node->__Type)
            {
                bRemoved = true;
                for (const FInputAxisKeyMapping& Mapping : GetProjectAxisMappings())
                {
                    if (Mapping.__AxisName == Node->__MappingName)
                        bRemoved = RemoveProjectAxisMapping(Mapping) && bRemoved;
                }
            }

            if (false == bRemoved)
            {
                MessageBoxW(hDlg, L"The selected mapping could not be removed.",
                    L"Key Mapping", MB_OK | MB_ICONERROR);
                return TRUE;
            }

            RefreshMappingTree(hDlg);
            return TRUE;
        }

        if (IDOK == LOWORD(wParam) || IDCANCEL == LOWORD(wParam))
        {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;

    case WM_NOTIFY:
        if (0 != lParam && IDC_MAPPING_LIST == static_cast<int>(
            reinterpret_cast<NMHDR*>(lParam)->idFrom) &&
            TVN_SELCHANGEDW == reinterpret_cast<NMHDR*>(lParam)->code)
        {
            const FMappingTreeNode* Node = GetSelectedMappingTreeNode(hDlg);
            if (nullptr == Node)
                return TRUE;

            if (EMappingTreeNodeType::ActionMapping == Node->__Type ||
                EMappingTreeNodeType::ActionKey == Node->__Type)
                SendDlgItemMessageW(hDlg, IDC_MAPPING_TYPE, CB_SETCURSEL, 0, 0);
            else if (EMappingTreeNodeType::AxisMapping == Node->__Type ||
                EMappingTreeNodeType::AxisKey == Node->__Type)
                SendDlgItemMessageW(hDlg, IDC_MAPPING_TYPE, CB_SETCURSEL, 1, 0);
            else
                return TRUE;

            SetDlgItemTextA(hDlg, IDC_MAPPING_NAME, Node->__MappingName.c_str());
            if (EMappingTreeNodeType::ActionKey == Node->__Type)
            {
                const std::string KeyName(GetKeyName(Node->__ActionMapping.__Key));
                SetDlgItemTextA(hDlg, IDC_MAPPING_KEY, KeyName.c_str());
                CapturedMappingKey = Node->__ActionMapping.__Key;
            }
            else if (EMappingTreeNodeType::AxisKey == Node->__Type)
            {
                const std::string KeyName(GetKeyName(Node->__AxisMapping.__Key));
                SetDlgItemTextA(hDlg, IDC_MAPPING_KEY, KeyName.c_str());
                CapturedMappingKey = Node->__AxisMapping.__Key;
                char Scale[32] = {};
                std::snprintf(Scale, std::size(Scale), "%g", Node->__AxisMapping.__Scale);
                SetDlgItemTextA(hDlg, IDC_MAPPING_SCALE, Scale);
            }
            return TRUE;
        }
        break;
    }

    return FALSE;
}
