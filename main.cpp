#if defined(UNICODE) && !defined(_UNICODE)
#define _UNICODE
#elif defined(_UNICODE) && !defined(UNICODE)
#define UNICODE
#endif
#define MAX_SPLINE_POINTS 4

#include <tchar.h>
#include <windows.h>
#include <cmath>
#include <vector>
#include <string>
#include <fstream>
#include "draw.h"
#include <gdiplus.h>
#pragma comment (lib, "gdiplus.lib")
using namespace Gdiplus;


using namespace std;

// ===============================================================
// Data Structures and Global Variables
struct Shape {
    string type;
    int x1, y1, x2, y2;
    COLORREF color;
};
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT  num = 0;
    UINT  size = 0;
    GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;

    ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if (!pImageCodecInfo) return -1;

    GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    }

    free(pImageCodecInfo);
    return -1;
}


vector<Shape> shapes;
bool showCursor = false;
bool drawSpline = false;
bool fillQuarter = false;
int currentQuarter = 1;

int mouseX = 0, mouseY = 0;
static vector<POINT> splinePoints;

// ===============================================================
// Function Definitions
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
                    shapes.push_back({"Circle", x - smallR, y - smallR, x + smallR, y + smallR, color});
                }
            }
        }
    }

    DeleteObject(pen);
    DeleteObject(brush);
}


void SaveShapesToFile(const string& filename) {
    HWND hwnd = GetForegroundWindow();
    RECT rc;
    GetClientRect(hwnd, &rc);

    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    HDC hdcWindow = GetDC(hwnd);
    HDC hdcMemDC = CreateCompatibleDC(hdcWindow);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcWindow, width, height);
    SelectObject(hdcMemDC, hBitmap);

    BitBlt(hdcMemDC, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY);


    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    Bitmap bitmap(hBitmap, NULL);

    CLSID pngClsid;
    GetEncoderClsid(L"image/png", &pngClsid);

    bitmap.Save(L"drawing.png", &pngClsid, NULL);

    GdiplusShutdown(gdiplusToken);


    DeleteObject(hBitmap);
    DeleteDC(hdcMemDC);
    ReleaseDC(hwnd, hdcWindow);

    MessageBox(NULL, _T("Current drawing saved as 'drawing.png'"), _T("Saved"), MB_OK);
}

enum CircleAlgorithm {
    CIRCLE_DIRECT, CIRCLE_POLAR, CIRCLE_ITER_POLAR, CIRCLE_MIDPOINT, CIRCLE_MOD_MID, NUL
};

CircleAlgorithm currentCircleAlgorithm = NUL;

void DrawPixel(HDC hdc, int x, int y, COLORREF color) {
    SetPixel(hdc, x, y, color);
}

void Draw8Points(HDC hdc, int xc, int yc, int x, int y, COLORREF color) {
    DrawPixel(hdc, xc + x, yc + y, color);
    DrawPixel(hdc, xc - x, yc + y, color);
    DrawPixel(hdc, xc + x, yc - y, color);
    DrawPixel(hdc, xc - x, yc - y, color);
    DrawPixel(hdc, xc + y, yc + x, color);
    DrawPixel(hdc, xc - y, yc + x, color);
    DrawPixel(hdc, xc + y, yc - x, color);
    DrawPixel(hdc, xc - y, yc - x, color);
}

void DrawCircle_Direct(HDC hdc, int xc, int yc, int R, COLORREF color) {
    for (int x = 0; x <= R / sqrt(2); ++x) {
        int y = round(sqrt(R * R - x * x));
        Draw8Points(hdc, xc, yc, x, y, color);
    }
}

void DrawCircle_Polar(HDC hdc, int xc, int yc, int R, COLORREF color) {
    double dtheta = 1.0 / R;
    for (double theta = 0; theta <= M_PI / 4; theta += dtheta) {
        int x = round(R * cos(theta));
        int y = round(R * sin(theta));
        Draw8Points(hdc, xc, yc, x, y, color);
    }
}

void DrawCircle_IterativePolar(HDC hdc, int xc, int yc, int R, COLORREF color) {
    double x = R, y = 0;
    double dtheta = 1.0 / R;
    double cos_dtheta = cos(dtheta), sin_dtheta = sin(dtheta);
    for (int i = 0; i <= R * M_PI / 4; ++i) {
        Draw8Points(hdc, xc, yc, round(x), round(y), color);
        double x_new = x * cos_dtheta - y * sin_dtheta;
        y = x * sin_dtheta + y * cos_dtheta;
        x = x_new;
    }
}

void DrawCircle_Midpoint(HDC hdc, int xc, int yc, int R, COLORREF color) {
    int x = 0, y = R;
    int d = 1 - R;
    while (x < y) {
        Draw8Points(hdc, xc, yc, x, y, color);
        if (d < 0) d += 2 * x + 3;
        else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

void DrawCircle_ModifiedMidpoint(HDC hdc, int xc, int yc, int R, COLORREF color) {
    int x = 0, y = R;
    int d = 1 - R;
    int dE = 3, dSE = -2 * R + 5;
    while (x <= y) {
        Draw8Points(hdc, xc, yc, x, y, color);
        if (d < 0) {
            d += dE;
            dE += 2;
            dSE += 2;
        } else {
            d += dSE;
            dE += 2;
            dSE += 4;
            y--;
        }
        x++;
    }
}

#define ID_COLOR_RED    101
#define ID_COLOR_GREEN  102
#define ID_COLOR_BLUE   103
#define ID_COLOR_BLACK  104

COLORREF currentColor = RGB(0, 0, 0);

// ===============================================================
// GUI Definitions
#define ID_CLEAR_SCREEN   1
#define ID_SET_WHITE_BG   2
#define ID_SAVE_SHAPES    3
#define ID_DRAW_CURSOR    4
#define ID_DRAW_SPLINE    5
#define ID_FILL_QUARTER   6
#define ID_ALGO_DIRECT    7
#define ID_ALGO_POLAR     8
#define ID_ALGO_ITER_POLAR 9
#define ID_ALGO_MIDPOINT  10
#define ID_ALGO_MOD_MID   11

LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);
TCHAR szClassName[] = _T("2D Drawing App");

HBRUSH backgroundBrush = (HBRUSH) GetStockObject(WHITE_BRUSH);

HMENU CreateMainMenu() {
    HMENU hMenu = CreateMenu();
    HMENU hSubMenu = CreatePopupMenu();

    AppendMenu(hSubMenu, MF_STRING, ID_SET_WHITE_BG, _T("Change background to white"));
    AppendMenu(hSubMenu, MF_STRING, ID_CLEAR_SCREEN, _T("Clear screen"));
    AppendMenu(hSubMenu, MF_STRING, ID_SAVE_SHAPES, _T("Save shapes to file"));
    AppendMenu(hSubMenu, MF_STRING, ID_DRAW_CURSOR, _T("Draw Custom Cursor"));
    AppendMenu(hSubMenu, MF_STRING, ID_DRAW_SPLINE, _T("Draw Cardinal Spline"));
    AppendMenu(hSubMenu, MF_STRING, ID_FILL_QUARTER, _T("Fill Circle Quarter with Circles"));
    AppendMenu(hSubMenu, MF_STRING, ID_ALGO_DIRECT, _T("Circle Direct"));
    AppendMenu(hSubMenu, MF_STRING, ID_ALGO_POLAR, _T("Circle Polar"));
    AppendMenu(hSubMenu, MF_STRING, ID_ALGO_ITER_POLAR, _T("Circle Iterative Polar"));
    AppendMenu(hSubMenu, MF_STRING, ID_ALGO_MIDPOINT, _T("Circle Midpoint"));
    AppendMenu(hSubMenu, MF_STRING, ID_ALGO_MOD_MID, _T("Circle Modified Midpoint"));
    AppendMenu(hSubMenu, MF_STRING, ID_COLOR_RED, _T("Change to Red"));
    AppendMenu(hSubMenu, MF_STRING, ID_COLOR_GREEN, _T("Change to Green"));
    AppendMenu(hSubMenu, MF_STRING, ID_COLOR_BLUE, _T("Change to Blue"));
    AppendMenu(hSubMenu, MF_STRING, ID_COLOR_BLACK, _T("Change to Black"));

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, _T("Options"));

    return hMenu;
}

int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nCmdShow) {
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
    wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

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

int x;
int y;
HDC hdc;
PAINTSTRUCT ps;

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_MOUSEMOVE:
            mouseX = LOWORD(lParam);
            mouseY = HIWORD(lParam);

            x = mouseX;
            y = mouseY;

            InvalidateRect(hwnd, NULL, TRUE);
            break;


        case WM_LBUTTONDOWN:
            if (drawSpline && splinePoints.size() < MAX_SPLINE_POINTS) {
                int x = LOWORD(lParam);
                int y = HIWORD(lParam);
                splinePoints.push_back({ x, y });

                if (splinePoints.size() == MAX_SPLINE_POINTS) {

                    drawSpline = false;
                }

                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;


        case WM_PAINT:
        {
            PAINTSTRUCT p;
            HDC hdc = BeginPaint(hwnd, &p);

            if (drawSpline && splinePoints.size() == MAX_SPLINE_POINTS) {
                DrawCardinalSpline(hdc, splinePoints.data(), splinePoints.size(), currentColor);
            }


            if (showCursor) DrawCustomCursor(hdc, mouseX, mouseY);
            if (splinePoints.size() == MAX_SPLINE_POINTS) {
                DrawCardinalSpline(hdc, splinePoints.data(), splinePoints.size(), currentColor);
            }
            if (fillQuarter) FillQuarterWithCircles(hdc, 250, 250, 100, currentQuarter);
            if (currentCircleAlgorithm != NUL) {
                switch (currentCircleAlgorithm) {
                    case CIRCLE_DIRECT:
                        DrawCircle_Direct(hdc, x, y, 50, currentColor);
                        break;
                    case CIRCLE_POLAR:
                        DrawCircle_Polar(hdc, x, y, 50, currentColor);
                        break;
                    case CIRCLE_ITER_POLAR:
                        DrawCircle_IterativePolar(hdc, x, y, 50, currentColor);
                        break;
                    case CIRCLE_MIDPOINT:
                        DrawCircle_Midpoint(hdc, x, y, 50, currentColor);
                        break;
                    case CIRCLE_MOD_MID:
                        DrawCircle_ModifiedMidpoint(hdc, x, y, 50, currentColor);
                        break;
                    default:
                        break;
                }
            }

            EndPaint(hwnd, &p);
        }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_CLEAR_SCREEN:
                    shapes.clear();
                    showCursor = false;
                    drawSpline = false;
                    fillQuarter = false;
                    currentCircleAlgorithm = NUL;
                    splinePoints.clear();
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;

                case ID_SET_WHITE_BG:
                    backgroundBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
                    SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)backgroundBrush);
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;

                case ID_SAVE_SHAPES:
                    SaveShapesToFile("shapes.txt");
                    break;

                case ID_DRAW_CURSOR:
                    showCursor = !showCursor;
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;

                case ID_DRAW_SPLINE:
                    drawSpline = true;
                    splinePoints.clear();
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;


                case ID_FILL_QUARTER:
                    currentQuarter = (currentQuarter % 4) + 1;
                    fillQuarter = true;
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;

                case ID_ALGO_DIRECT:      currentCircleAlgorithm = CIRCLE_DIRECT; break;

                case ID_ALGO_POLAR:       currentCircleAlgorithm = CIRCLE_POLAR; break;

                case ID_ALGO_ITER_POLAR:  currentCircleAlgorithm = CIRCLE_ITER_POLAR; break;

                case ID_ALGO_MIDPOINT:    currentCircleAlgorithm = CIRCLE_MIDPOINT; break;

                case ID_ALGO_MOD_MID:     currentCircleAlgorithm = CIRCLE_MOD_MID; break;

                case ID_COLOR_RED:
                    currentColor = RGB(255, 0, 0);
                    break;

                case ID_COLOR_GREEN:
                    currentColor = RGB(0, 255, 0);
                    break;

                case ID_COLOR_BLUE:
                    currentColor = RGB(0, 0, 255);
                    break;

                case ID_COLOR_BLACK:
                    currentColor = RGB(0, 0, 0);
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