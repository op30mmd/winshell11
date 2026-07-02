#pragma once
#include <windows.h>

namespace shell::ui {

// Shared visual style for all widgets. Owned by UiHost and passed to
// Widget::Paint so a whole window can be re-themed in one place.
struct Theme {
    COLORREF background = RGB(32, 32, 32);
    COLORREF surface = RGB(45, 45, 45);
    COLORREF surfaceHover = RGB(60, 60, 60);
    COLORREF surfacePressed = RGB(75, 75, 75);
    COLORREF accent = RGB(0, 120, 215);
    COLORREF text = RGB(240, 240, 240);
    COLORREF textSecondary = RGB(170, 170, 170);
    COLORREF border = RGB(70, 70, 70);

    int cornerRadius = 6;
    int fontSize = 15;
    const wchar_t* fontFamily = L"Segoe UI";
};

} // namespace shell::ui
