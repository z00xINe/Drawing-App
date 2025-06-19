#ifndef HERMITE_H
#define HERMITE_H

#include <windows.h>
#include <cmath>

// Draw Hermite curve between two points with tangents
void DrawHermite(HDC hdc, POINT p0, POINT p1, POINT t0, POINT t1, COLORREF color = RGB(255, 0, 0)) {
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ oldPen = SelectObject(hdc, pen);

    for (double t = 0; t <= 1.0; t += 0.001) {
        double t2 = t * t;
        double t3 = t2 * t;

        double h1 = 2 * t3 - 3 * t2 + 1;
        double h2 = -2 * t3 + 3 * t2;
        double h3 = t3 - 2 * t2 + t;
        double h4 = t3 - t2;

        double x = h1 * p0.x + h2 * p1.x + h3 * t0.x + h4 * t1.x;
        double y = h1 * p0.y + h2 * p1.y + h3 * t0.y + h4 * t1.y;

        SetPixel(hdc, (int)x, (int)y, color);
    }

    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

void FillSquareWithHermiteCurvesVertical(HDC hdc, int x, int y, int size, COLORREF color = RGB(0, 0, 0)) {
    int spacing = 10;

    for (int i = x; i <= x + size; i += spacing) {
        POINT p0 = {i, y};
        POINT p1 = {i, y + size};

        // vertical tangents for curve shaping
        POINT t0 = {0, size / 2};
        POINT t1 = {0, -size / 2};

        // int offset = i - (x + size / 2);  // how far from center of square
        // POINT t0 = {offset / 2, size / 2};
        // POINT t1 = {-offset / 2, -size / 2};


        DrawHermite(hdc, p0, p1, t0, t1, color);
    }
}

#endif
