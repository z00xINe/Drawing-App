#ifndef ELLIPSE_H
#define ELLIPSE_H

#include <windows.h>
#include <cmath>

void Draw4Points(HDC hdc, int xc, int yc, int x, int y, COLORREF color) {
    SetPixel(hdc, xc + x, yc + y, color);
    SetPixel(hdc, xc - x, yc + y, color);
    SetPixel(hdc, xc + x, yc - y, color);
    SetPixel(hdc, xc - x, yc - y, color);
}


void DrawEllipse_Direct(HDC hdc, int xc, int yc, int a, int b, COLORREF color = RGB(165, 42, 42)) {
    float x, y;

    for (x = 0; x <= a; ++x) {
        y = b * sqrt(1.0 - (x * x) / (float)(a * a));
        Draw4Points(hdc, xc, yc, (int)(x + 0.5), (int)(y + 0.5), color);
    }

    for (y = 0; y <= b; ++y) {
        x = a * sqrt(1.0 - (y * y) / (float)(b * b));
        Draw4Points(hdc, xc, yc, (int)(x + 0.5), (int)(y + 0.5), color);
    }
}

void DrawEllipse_Polar(HDC hdc, int xc, int yc, int a, int b, COLORREF color = RGB(0, 255, 255)) {
    const float PI = 3.14159265f;
    float theta = 0;
    float step = 1.0f / std::max(a, b);

    while (theta <= 2 * PI) {
        int x = (int)(a * cos(theta));
        int y = (int)(b * sin(theta));
        SetPixel(hdc, xc + x, yc + y, color);
        theta += step;
    }
}

void DrawEllipse_Midpoint(HDC hdc, int xc, int yc, int a, int b, COLORREF color = RGB(255, 0, 255)) {
    int x = 0;
    int y = b;

    int aSquared = a * a;
    int bSquared = b * b;
    int twoASquared = 2 * aSquared;
    int twoBSquared = 2 * bSquared;

    int dx = 0;
    int dy = twoASquared * y;

    // Region 1
    int p1 = round(bSquared - aSquared * b + 0.25 * aSquared);
    while (dx < dy) {
        Draw4Points(hdc, xc, yc, x, y, color);
        x++;
        dx += twoBSquared;
        if (p1 < 0) {
            p1 += bSquared + dx;
        } else {
            y--;
            dy -= twoASquared;
            p1 += bSquared + dx - dy;
        }
    }

    // Region 2
    int p2 = round(bSquared * (x + 0.5) * (x + 0.5) + aSquared * (y - 1)*(y - 1) - aSquared * bSquared);
    while (y >= 0) {
        Draw4Points(hdc, xc, yc, x, y, color);
        y--;
        dy -= twoASquared;
        if (p2 > 0) {
            p2 += aSquared - dy;
        } else {
            x++;
            dx += twoBSquared;
            p2 += aSquared - dy + dx;
        }
    }
}

#endif
