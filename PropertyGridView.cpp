#include "framework.h"
#include "PropertyGridView.h"

#define MAX_LOADSTRING 100

HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

PROPERTY_GRID pgData;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PROPERTYGRIDVIEW, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PROPERTYGRIDVIEW));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PROPERTYGRIDVIEW));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_PROPERTYGRIDVIEW);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_VSCROLL,
      CW_USEDEFAULT, 0, 400, 700, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   InitPropertyGridView(&pgData, hWnd, hInstance);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
        {
            break;
        }
        case WM_ERASEBKGND:
        {
            RECT rc;
            GetClientRect(hWnd, &rc);
            FillRect((HDC)wParam, &rc, pgData.brBackground);
            return 1;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            OnPaint(&pgData, hdc, &ps.rcPaint, ps.fErase);
            EndPaint(hWnd, &ps);
            break;
        }
        case WM_SIZE:
        {
            OnSize(&pgData, LOWORD(lParam), HIWORD(lParam));
            break;
        }
        case WM_VSCROLL:
        {
            OnScroll(&pgData, LOWORD(wParam), 5);
            break;
        }
        case WM_MOUSEMOVE:
        {
            OnMouseMove(&pgData, wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;
        }
        case WM_MOUSELEAVE:
        {
            // reserved
            break;
        }
        case WM_MOUSEHOVER:
        {
            // reserved
            break;
        }
        case WM_LBUTTONDOWN:
        {
            OnMouseLeftButtonPress(&pgData, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;
        }
        case WM_LBUTTONUP:
        {
            pgData.bDragging &= 0;
            break;
        }
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        {
            // Value edit control color set up
            if (pgData.itemSelect)
            {
                HDC hdc = (HDC)wParam;
                SetTextColor(hdc, pgData.itemSelect->clrVal);
                SetBkColor(hdc, pgData.clrBackground);
            }
            return (INT_PTR)(pgData.brBackground);
        }
        case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case IDM_LAUNCHTEST:
            {
                // TODO
                HPROPERTY cat1 = PropertyGridNewCategory(&pgData, L"Category 1");
                HPROPERTY cat2 = PropertyGridNewCategory(&pgData, L"Category 2");
                HPROPERTY cat3 = PropertyGridNewCategory(&pgData, L"Category 3");
                HPROPERTY cat4 = PropertyGridNewCategory(&pgData, L"Category 4");
                PropertyGridAddProperty(&pgData, PropertyGridNewProperty(&pgData, L"Test 1", L"Value 1"));
                PropertyGridAddProperty(&pgData, cat1);
                PropertyGridAddSubItem(&pgData, cat1, PropertyGridNewProperty(&pgData, L"Test 2", L"Value 2"));
                PropertyGridAddSubItem(&pgData, cat1, cat2);
                PropertyGridAddSubItem(&pgData, cat2, PropertyGridNewProperty(&pgData, L"Test 3", L"Value 3"));
                PropertyGridAddSubItem(&pgData, cat2, PropertyGridNewProperty(&pgData, L"Test 4", L"Value 4"));
                PropertyGridAddSubItem(&pgData, cat2, PropertyGridNewProperty(&pgData, L"Test 5", L"Value 5"));
                PropertyGridAddSubItem(&pgData, cat2, PropertyGridNewProperty(&pgData, L"Test 6", L"Value 6"));
                PropertyGridAddProperty(&pgData, cat3);
                PropertyGridAddSubItem(&pgData, cat3, PropertyGridNewProperty(&pgData, L"Test 7", L"Value"));
                PropertyGridAddSubItem(&pgData, cat3, PropertyGridNewProperty(&pgData, L"Test 8", L"Value"));
                PropertyGridAddSubItem(&pgData, cat3, PropertyGridNewProperty(&pgData, L"Test 9", L"Value"));
                PropertyGridAddSubItem(&pgData, cat3, PropertyGridNewProperty(&pgData, L"Test 10", L"Value"));
                PropertyGridAddProperty(&pgData, cat4);
                PropertyGridAddSubItem(&pgData, cat4, PropertyGridNewProperty(&pgData, L"Test 11", L"Value"));
                PropertyGridAddSubItem(&pgData, cat4, PropertyGridNewProperty(&pgData, L"Test 12", L"Value"));
                PropertyGridAddSubItem(&pgData, cat4, PropertyGridNewProperty(&pgData, L"Test 13", L"Value"));
                PropertyGridAddSubItem(&pgData, cat4, PropertyGridNewProperty(&pgData, L"Test 14", L"Value"));
                PropertyGridAddSubItem(&pgData, cat2, PropertyGridNewProperty(&pgData, L"Add Test", L"Value"));
                // Delete Test
                // PropertyGridDeleteProperty(&pgData, cat3);
                // Disable Test
                HPROPERTY propDisable = PropertyGridNewProperty(&pgData, L"(Name Disable)", L"Value Disable");
                PropertyGridAddSubItem(&pgData, cat3, propDisable);
                PropertyGridDisableProperty(&pgData, propDisable, TRUE);
                break;
            }
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;
        }
        case WM_DESTROY:
            ReleasePropertyGridView(&pgData);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK ValueEditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_KEYDOWN:
        {
            switch (wParam)
            {
                case VK_RETURN:
                case VK_TAB:
                {
                    HPROPERTY propNext = PropertyGridItemGetNextVisible(&pgData, pgData.itemSelect, 0, pgData.szControl.cy);
                    while (isCategory(propNext))
                    {
                        propNext = PropertyGridItemGetNextVisible(&pgData, (PROPERTY_ITEM*)propNext, 0, pgData.szControl.cy);
                    }

                    PropertyGridSetSelection(&pgData, propNext, TRUE);
                    return 0;
                }
                case VK_ESCAPE:
                {
                    PropertyGridCancelSelection(&pgData, FALSE);
                    return 0;
                }
                default:
                    break;
            }
            // By default, we fall through into old proc, so no break here
        }
        default:
            return CallWindowProc(pgData.procOldEditControl, hwnd, msg, wParam, lParam);
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
