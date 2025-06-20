#ifndef UNTITLED6_CIRCLE_H
#define UNTITLED6_CIRCLE_H

#include <windows.h>
#include <cmath>

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

#endif
