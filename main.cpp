#if defined(UNICODE) && !defined(_UNICODE)
#define _UNICODE
#elif defined(_UNICODE) && !defined(UNICODE)
#define UNICODE
#endif
#define MAX_SPLINE_POINTS 4
#define ID_FILL_QUARTER_1 201
#define ID_FILL_QUARTER_2 202
#define ID_FILL_QUARTER_3 203
#define ID_FILL_QUARTER_4 204


#include <tchar.h>
#include <windows.h>
#include <cmath>
#include <vector>
#include <string>
#include <fstream>
#include "draw.h"
#include "hermite.h"
#include "ellipse.h"
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

bool drawEllipseDirect = false;
bool drawEllipsePolar = false;
bool drawEllipseMidpoint = false;
bool fillHermite = false;

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

void LoadDataFromFile() {
    ifstream file("myshapes.txt");
    if (!file.is_open()) {
        MessageBox(NULL, _T("Failed to open file."), _T("Load Error"), MB_OK);
        return;
    }

    shapes.clear(); //clear previous shapes before loading

    string type;
    int x, y, a, b;
//    COLORREF color;

    while (file >> type >> x >> y >> a >> b ) {
        shapes.push_back({type, x, y, a, b});
    }

    file.close();
    MessageBox(NULL, _T("Shapes loaded from file"), _T("Load"), MB_OK);

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
#define ID_FILL_HERMITE 20
#define ID_DRAW_ELLIPSE_DIRECT    21
#define ID_DRAW_ELLIPSE_POLAR     22
#define ID_DRAW_ELLIPSE_MIDPOINT  23
#define ID_LOAD_SHAPES 24

LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);
TCHAR szClassName[] = _T("2D Drawing App");

HBRUSH backgroundBrush = (HBRUSH) GetStockObject(WHITE_BRUSH);

HMENU CreateMainMenu() {
    HMENU hMenu = CreateMenu();
    HMENU hSubMenu = CreatePopupMenu();

    AppendMenu(hSubMenu, MF_STRING, ID_SET_WHITE_BG, _T("Change background to white"));
    AppendMenu(hSubMenu, MF_STRING, ID_CLEAR_SCREEN, _T("Clear screen"));
    AppendMenu(hSubMenu, MF_STRING, ID_SAVE_SHAPES, _T("Save shapes to file"));
    AppendMenu(hSubMenu, MF_STRING, ID_LOAD_SHAPES, _T("Load shapes from file"));
    AppendMenu(hSubMenu, MF_STRING, ID_DRAW_CURSOR, _T("Draw Custom Cursor"));
    AppendMenu(hSubMenu, MF_STRING, ID_DRAW_SPLINE, _T("Draw Cardinal Spline"));
    AppendMenu(hSubMenu, MF_STRING, ID_FILL_QUARTER_1, _T("Fill Quarter 1"));
    AppendMenu(hSubMenu, MF_STRING, ID_FILL_QUARTER_2, _T("Fill Quarter 2"));
    AppendMenu(hSubMenu, MF_STRING, ID_FILL_QUARTER_3, _T("Fill Quarter 3"));
    AppendMenu(hSubMenu, MF_STRING, ID_FILL_QUARTER_4, _T("Fill Quarter 4"));
    AppendMenu(hSubMenu, MF_STRING, ID_ALGO_DIRECT, _T("Circle Direct"));
    AppendMenu(hSubMenu, MF_STRING, ID_ALGO_POLAR, _T("Circle Polar"));
    AppendMenu(hSubMenu, MF_STRING, ID_ALGO_ITER_POLAR, _T("Circle Iterative Polar"));
    AppendMenu(hSubMenu, MF_STRING, ID_ALGO_MIDPOINT, _T("Circle Midpoint"));
    AppendMenu(hSubMenu, MF_STRING, ID_ALGO_MOD_MID, _T("Circle Modified Midpoint"));
    AppendMenu(hSubMenu, MF_STRING, ID_COLOR_RED, _T("Change to Red"));
    AppendMenu(hSubMenu, MF_STRING, ID_COLOR_GREEN, _T("Change to Green"));
    AppendMenu(hSubMenu, MF_STRING, ID_COLOR_BLUE, _T("Change to Blue"));
    AppendMenu(hSubMenu, MF_STRING, ID_COLOR_BLACK, _T("Change to Black"));
    AppendMenu(hSubMenu, MF_STRING, ID_FILL_HERMITE, _T("Fill Square with Vertical Hermite Curves"));
    AppendMenu(hSubMenu, MF_STRING, ID_DRAW_ELLIPSE_DIRECT, _T("Draw Ellipse (Direct)"));
    AppendMenu(hSubMenu, MF_STRING, ID_DRAW_ELLIPSE_POLAR, _T("Draw Ellipse (Polar)"));
    AppendMenu(hSubMenu, MF_STRING, ID_DRAW_ELLIPSE_MIDPOINT, _T("Draw Ellipse (Midpoint)"));

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

            // x = mouseX;
            //y = mouseY;

            InvalidateRect(hwnd, NULL, FALSE);
            break;


        case WM_LBUTTONDOWN:
            x = LOWORD(lParam);
            y = HIWORD(lParam);

            if (drawSpline && splinePoints.size() < MAX_SPLINE_POINTS) {
                POINT pt = { x, y };
                splinePoints.push_back(pt);
                if (splinePoints.size() == MAX_SPLINE_POINTS) drawSpline = false;
            }
            else if (currentCircleAlgorithm != NUL) {

                shapes.clear();

                shapes.push_back({ "Circle", x - 50, y - 50, x + 50, y + 50, currentColor });
            }

            InvalidateRect(hwnd, NULL, TRUE);
            break;




        case WM_PAINT:
        {
            PAINTSTRUCT p;
            HDC hdc = BeginPaint(hwnd, &p);

            if (showCursor)
                DrawCustomCursor(hdc, mouseX, mouseY);

            if (drawEllipseDirect) DrawEllipse_Direct(hdc, 300, 200, 100, 50);
            if (drawEllipsePolar) DrawEllipse_Polar(hdc, 300, 200, 100, 50);
            if (drawEllipseMidpoint) DrawEllipse_Midpoint(hdc, 300, 200, 100, 50);
            if (fillHermite) FillSquareWithHermiteCurvesVertical(hdc, 100, 100, 100);

            // draw loaded shapes
            for (const auto& shape : shapes) {
                if (shape.type == "Ellipse")
                    DrawEllipse_Direct(hdc, shape.x1, shape.y1, shape.x2, shape.y2);
//                else if (shape.type == "")
//                    ;
            }


            if (splinePoints.size() == MAX_SPLINE_POINTS) {
                DrawCardinalSpline(hdc, splinePoints.data(), splinePoints.size(), currentColor);
            }


            for (const auto& shape : shapes) {
                if (shape.type == "Circle") {
                    int cx = (shape.x1 + shape.x2) / 2;
                    int cy = (shape.y1 + shape.y2) / 2;
                    int R = (shape.x2 - shape.x1) / 2;

                    switch (currentCircleAlgorithm) {
                        case CIRCLE_DIRECT:
                            DrawCircle_Direct(hdc, cx, cy, R, shape.color); break;
                        case CIRCLE_POLAR:
                            DrawCircle_Polar(hdc, cx, cy, R, shape.color); break;
                        case CIRCLE_ITER_POLAR:
                            DrawCircle_IterativePolar(hdc, cx, cy, R, shape.color); break;
                        case CIRCLE_MIDPOINT:
                            DrawCircle_Midpoint(hdc, cx, cy, R, shape.color); break;
                        case CIRCLE_MOD_MID:
                            DrawCircle_ModifiedMidpoint(hdc, cx, cy, R, shape.color); break;
                        default: break;
                    }

                    if (fillQuarter && &shape == &shapes.back()) {
                        FillQuarterWithCircles(hdc, cx, cy, R, currentQuarter);
                    }

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
                    fillHermite = false;
                    drawEllipseDirect = drawEllipsePolar = drawEllipseMidpoint = false;
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

                case ID_LOAD_SHAPES:
                    LoadDataFromFile();
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;

                case ID_FILL_HERMITE:
                    fillHermite = true;
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;


                case ID_DRAW_ELLIPSE_DIRECT:
                    drawEllipseDirect = true;
                    drawEllipsePolar = drawEllipseMidpoint = false;
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;

                case ID_DRAW_ELLIPSE_POLAR:
                    drawEllipsePolar = true;
                    drawEllipseDirect = drawEllipseMidpoint = false;
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;

                case ID_DRAW_ELLIPSE_MIDPOINT:
                    drawEllipseMidpoint = true;
                    drawEllipseDirect = drawEllipsePolar = false;
                    InvalidateRect(hwnd, NULL, TRUE);
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


                case ID_FILL_QUARTER_1:
                case ID_FILL_QUARTER_2:
                case ID_FILL_QUARTER_3:
                case ID_FILL_QUARTER_4:
                    fillQuarter = true;
                    currentQuarter = LOWORD(wParam) - ID_FILL_QUARTER_1 + 1;
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
