#if defined(UNICODE) && !defined(_UNICODE)
#define _UNICODE
#elif defined(_UNICODE) && !defined(UNICODE)
#define UNICODE
#endif

#include <tchar.h>
#include <windows.h>

#define ID_CLEAR_SCREEN 1
#define ID_SET_WHITE_BG 6

LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);
TCHAR szClassName[] = _T("2D Drawing App");

HBRUSH backgroundBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);

HMENU CreateMainMenu() {
    HMENU hMenu = CreateMenu();
    HMENU hSubMenu = CreatePopupMenu();

    AppendMenu(hSubMenu, MF_STRING, ID_SET_WHITE_BG, _T("Change the background of window to be white"));
    AppendMenu(hSubMenu, MF_STRING, ID_CLEAR_SCREEN, _T("Clear screen from shapes"));

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, _T("Options"));
    return hMenu;
}

int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance,
                   LPSTR lpszArgument, int nCmdShow)
{
    HWND hwnd;
    MSG messages;
    WNDCLASSEX wincl;

    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;
    wincl.style = CS_DBLCLKS;
    wincl.cbSize = sizeof(WNDCLASSEX);

    wincl.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wincl.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;
    wincl.cbClsExtra = 0;
    wincl.cbWndExtra = 0;
    wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    if (!RegisterClassEx(&wincl))
        return 0;

    HMENU hMenu = CreateMainMenu();

    hwnd = CreateWindowEx(
            0, szClassName,
            _T("2D Drawing App"),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            544, 375,
            HWND_DESKTOP, hMenu,
            hThisInstance, NULL
    );

    ShowWindow(hwnd, nCmdShow);

    while (GetMessage(&messages, NULL, 0, 0)) {
        TranslateMessage(&messages);
        DispatchMessage(&messages);
    }

    return messages.wParam;
}

bool pointSet = false;

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    PAINTSTRUCT p;

    switch (message) {
        case WM_PAINT:
            BeginPaint(hwnd, &p);

            EndPaint(hwnd, &p);
            break;
        case WM_LBUTTONDOWN:

            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_CLEAR_SCREEN:
                    pointSet = false;
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
                case ID_SET_WHITE_BG:
                    backgroundBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
                    SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)backgroundBrush);
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}