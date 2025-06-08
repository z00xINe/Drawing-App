#ifndef DRAW_H
#define DRAW_H

#include <windows.h>
#include <vector>

void DrawCardinalSpline(HDC hdc, POINT* pts, int count, float tension = 0.5f, COLORREF color = RGB(0, 128, 0)) {
    if (count < 2) return;

    HPEN pen = CreatePen(PS_SOLID, 2, color);
    HGDIOBJ oldPen = SelectObject(hdc, pen);

    std::vector<POINT> extendedPoints;
    extendedPoints.reserve(count + 2);

    extendedPoints.push_back({ 2 * pts[0].x - pts[1].x, 2 * pts[0].y - pts[1].y });

    for (int i = 0; i < count; i++) {
        extendedPoints.push_back(pts[i]);
    }

    extendedPoints.push_back({ 2 * pts[count - 1].x - pts[count - 2].x, 2 * pts[count - 1].y - pts[count - 2].y });

    int n = (int)extendedPoints.size();

    for (int i = 1; i < n - 2; i++) {
        for (float t = 0; t <= 1.0f; t += 0.01f) {

            float t2 = t * t;
            float t3 = t2 * t;

            float s = (1 - tension) / 2;

            float b1 = -s * t3 + 2 * s * t2 - s * t;
            float b2 = (2 - s) * t3 + (s - 3) * t2 + 1;
            float b3 = (s - 2) * t3 + (3 - 2 * s) * t2 + s * t;
            float b4 = s * t3 - s * t2;

            float x = b1 * extendedPoints[i - 1].x + b2 * extendedPoints[i].x + b3 * extendedPoints[i + 1].x + b4 * extendedPoints[i + 2].x;
            float y = b1 * extendedPoints[i - 1].y + b2 * extendedPoints[i].y + b3 * extendedPoints[i + 1].y + b4 * extendedPoints[i + 2].y;

            if (t == 0) {
                MoveToEx(hdc, (int)x, (int)y, NULL);
            } else {
                LineTo(hdc, (int)x, (int)y);
            }
        }
    }

    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

#endif // DRAW_H
