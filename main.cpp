#if defined(UNICODE) && !defined(_UNICODE)
#define _UNICODE
#elif defined(_UNICODE) && !defined(UNICODE)
#define UNICODE
#endif

#include <tchar.h>
#include <windows.h>
#include <cmath>
#include <vector>
#include <string>
#include <fstream>
#include "draw.h"

using namespace std;

struct Shape {
    string type;
    int x1, y1, x2, y2;
    COLORREF color;
};

vector<Shape> shapes;

void DrawCustomCursor(HDC hdc, int x, int y, COLORREF color = RGB(255, 0, 0)) {
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ oldPen = SelectObject(hdc, pen);

    MoveToEx(hdc, x - 5, y, NULL);
    LineTo(hdc, x + 5, y);
    MoveToEx(hdc, x, y - 5, NULL);
    LineTo(hdc, x, y + 5);

    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

void FillQuarterWithCircles(HDC hdc, int cx, int cy, int R, int quarter, COLORREF color = RGB(0, 0, 255)) {
    int smallR = 10;
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HBRUSH brush = CreateSolidBrush(color);
    SelectObject(hdc, pen);
    SelectObject(hdc, brush);

    for (int x = cx - R; x <= cx + R; x += smallR * 2) {
        for (int y = cy - R; y <= cy + R; y += smallR * 2) {
            double dist = sqrt((x - cx)*(x - cx) + (y - cy)*(y - cy));
            if (dist + smallR <= R) {
                bool inQuarter = false;
                switch (quarter) {
                    case 1: if (x >= cx && y <= cy) inQuarter = true; break;
                    case 2: if (x <= cx && y <= cy) inQuarter = true; break;
                    case 3: if (x <= cx && y >= cy) inQuarter = true; break;
                    case 4: if (x >= cx && y >= cy) inQuarter = true; break;
                }
                if (inQuarter) {
                    Ellipse(hdc, x - smallR, y - smallR, x + smallR, y + smallR);
                    shapes.push_back({"circle", x - smallR, y - smallR, x + smallR, y + smallR, color});
                }
            }
        }
    }

    DeleteObject(pen);
    DeleteObject(brush);
}

void SaveShapesToFile(const string& filename) {
    ofstream out(filename);
    if (!out.is_open()) return;

    for (const auto& shape : shapes) {
        out << shape.type << " " << shape.x1 << " " << shape.y1 << " "
            << shape.x2 << " " << shape.y2 << " " << shape.color << "\n";
    }

    out.close();
}

#define ID_CLEAR_SCREEN 1
#define ID_SET_WHITE_BG 6
#define ID_SAVE_SHAPES 7
#define ID_FILL_QUARTER 8

int mouseX = 0, mouseY = 0;
bool pointSet = false;
int currentQuarter = 1;

LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);
TCHAR szClassName[] = _T("2D Drawing App");
HBRUSH backgroundBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);

HMENU CreateMainMenu() {
    HMENU hMenu = CreateMenu();
    HMENU hSubMenu = CreatePopupMenu();

    AppendMenu(hSubMenu, MF_STRING, ID_SET_WHITE_BG, _T("Change background to white"));
    AppendMenu(hSubMenu, MF_STRING, ID_CLEAR_SCREEN, _T("Clear screen"));
    AppendMenu(hSubMenu, MF_STRING, ID_SAVE_SHAPES, _T("Save shapes to file"));
    AppendMenu(hSubMenu, MF_STRING, ID_FILL_QUARTER, _T("Fill Quarter with Circles"));

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, _T("Options"));
    return hMenu;
}

int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance,
                   LPSTR lpszArgument, int nCmdShow) {
    HWND hwnd;
    MSG messages;
    WNDCLASSEX wincl = {0};

    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;
    wincl.style = CS_DBLCLKS;
    wincl.cbSize = sizeof(WNDCLASSEX);
    wincl.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wincl.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;
    wincl.hbrBackground = backgroundBrush;

    if (!RegisterClassEx(&wincl)) return 0;

    HMENU hMenu = CreateMainMenu();
    hwnd = CreateWindowEx(0, szClassName, _T("2D Drawing App"),
                          WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                          600, 400, HWND_DESKTOP, hMenu, hThisInstance, NULL);

    ShowWindow(hwnd, nCmdShow);

    while (GetMessage(&messages, NULL, 0, 0)) {
        TranslateMessage(&messages);
        DispatchMessage(&messages);
    }

    return messages.wParam;
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static POINT pts[] = {{100, 100}, {150, 80}, {200, 120}, {250, 90}, {300, 150}};

    PAINTSTRUCT p;

    switch (message) {
        case WM_MOUSEMOVE:
            mouseX = LOWORD(lParam);
            mouseY = HIWORD(lParam);
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case WM_PAINT:
            BeginPaint(hwnd, &p);
            if (pointSet) {
                DrawCardinalSpline(p.hdc, pts, 5);
                FillQuarterWithCircles(p.hdc, 250, 250, 100, currentQuarter);
            }
            DrawCustomCursor(p.hdc, mouseX, mouseY);
            EndPaint(hwnd, &p);
            break;

        case WM_LBUTTONDOWN:
            pointSet = true;
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_CLEAR_SCREEN:
                    pointSet = false;
                    shapes.clear();
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;

                case ID_SET_WHITE_BG:
                    backgroundBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
                    SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)backgroundBrush);
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;

                case ID_SAVE_SHAPES:
                    SaveShapesToFile("shapes.txt");
                    MessageBox(hwnd, _T("Shapes saved to shapes.txt"), _T("Saved"), MB_OK);
                    break;

                case ID_FILL_QUARTER:
                    currentQuarter = (currentQuarter % 4) + 1;
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
