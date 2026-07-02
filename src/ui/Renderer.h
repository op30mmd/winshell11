#pragma once
#include <string>
#include <windows.h>

namespace shell::ui {

// Thin drawing helper over a GDI device context. Does not own the HDC.
// All coordinates are in window client space.
class Renderer {
public:
    explicit Renderer(HDC hdc) : m_hdc(hdc) {}

    HDC Handle() const { return m_hdc; }

    void FillRect(const RECT& rect, COLORREF color);
    void FillRoundedRect(const RECT& rect, COLORREF color, int radius);
    void FillEllipse(const RECT& rect, COLORREF color);
    void DrawBorder(const RECT& rect, COLORREF color, int radius);
    void DrawLine(int x1, int y1, int x2, int y2, COLORREF color, int width = 1);
    void DrawString(const std::wstring& text, const RECT& rect, COLORREF color, UINT format,
                    const wchar_t* fontFamily, int fontSize, bool bold = false);

private:
    HDC m_hdc = nullptr;
};

} // namespace shell::ui
