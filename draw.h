#ifndef DRAW_H
#define DRAW_H

#include <windows.h>

void DrawCardinalSpline(HDC hdc, POINT* pts, int count, float tension = 0.5f, COLORREF color = RGB(0, 128, 0)) {
    if (count < 2) return;

    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ oldPen = SelectObject(hdc, pen);

    for (int i = 1; i < count - 2; ++i) {
        for (float t = 0; t <= 1.0f; t += 0.01f) {
            float t2 = t * t;
            float t3 = t2 * t;

            float h1 = 2 * t3 - 3 * t2 + 1;
            float h2 = -2 * t3 + 3 * t2;
            float h3 = t3 - 2 * t2 + t;
            float h4 = t3 - t2;

            float x = h1 * pts[i].x + h2 * pts[i + 1].x + h3 * tension * (pts[i + 1].x - pts[i - 1].x) + h4 * tension * (pts[i + 2].x - pts[i].x);
            float y = h1 * pts[i].y + h2 * pts[i + 1].y + h3 * tension * (pts[i + 1].y - pts[i - 1].y) + h4 * tension * (pts[i + 2].y - pts[i].y);

            SetPixel(hdc, (int)x, (int)y, color);
        }
    }

    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

#endif // DRAW_H
