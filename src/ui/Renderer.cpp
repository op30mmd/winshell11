#include "Renderer.h"
#include <map>
#include <tuple>

namespace shell::ui {

namespace {

// Fonts are cached for the lifetime of the process. A shell host creates a
// small, fixed set of family/size/weight combinations, so the cache stays tiny.
HFONT GetCachedFont(const std::wstring& family, int size, bool bold) {
    static std::map<std::tuple<std::wstring, int, bool>, HFONT> s_cache;
    const auto key = std::make_tuple(family, size, bold);
    const auto it = s_cache.find(key);
    if (it != s_cache.end())
        return it->second;

    HFONT font = CreateFontW(-size, 0, 0, 0, bold ? FW_SEMIBOLD : FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                             family.c_str());
    s_cache[key] = font;
    return font;
}

} // namespace

void Renderer::FillRect(const RECT& rect, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    if (brush) {
        ::FillRect(m_hdc, &rect, brush);
        DeleteObject(brush);
    }
}

void Renderer::FillRoundedRect(const RECT& rect, COLORREF color, int radius) {
    if (radius <= 0) {
        FillRect(rect, color);
        return;
    }
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ oldBrush = SelectObject(m_hdc, brush);
    HGDIOBJ oldPen = SelectObject(m_hdc, pen);
    RoundRect(m_hdc, rect.left, rect.top, rect.right, rect.bottom, radius * 2, radius * 2);
    SelectObject(m_hdc, oldBrush);
    SelectObject(m_hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

void Renderer::FillEllipse(const RECT& rect, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ oldBrush = SelectObject(m_hdc, brush);
    HGDIOBJ oldPen = SelectObject(m_hdc, pen);
    Ellipse(m_hdc, rect.left, rect.top, rect.right, rect.bottom);
    SelectObject(m_hdc, oldBrush);
    SelectObject(m_hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

void Renderer::DrawBorder(const RECT& rect, COLORREF color, int radius) {
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ oldPen = SelectObject(m_hdc, pen);
    HGDIOBJ oldBrush = SelectObject(m_hdc, GetStockObject(HOLLOW_BRUSH));
    if (radius > 0)
        RoundRect(m_hdc, rect.left, rect.top, rect.right, rect.bottom, radius * 2, radius * 2);
    else
        Rectangle(m_hdc, rect.left, rect.top, rect.right, rect.bottom);
    SelectObject(m_hdc, oldBrush);
    SelectObject(m_hdc, oldPen);
    DeleteObject(pen);
}

void Renderer::DrawLine(int x1, int y1, int x2, int y2, COLORREF color, int width) {
    HPEN pen = CreatePen(PS_SOLID, width, color);
    HGDIOBJ oldPen = SelectObject(m_hdc, pen);
    MoveToEx(m_hdc, x1, y1, nullptr);
    LineTo(m_hdc, x2, y2);
    SelectObject(m_hdc, oldPen);
    DeleteObject(pen);
}

void Renderer::DrawString(const std::wstring& text, const RECT& rect, COLORREF color, UINT format,
                          const wchar_t* fontFamily, int fontSize, bool bold) {
    HFONT font = GetCachedFont(fontFamily, fontSize, bold);
    HGDIOBJ oldFont = SelectObject(m_hdc, font);
    SetBkMode(m_hdc, TRANSPARENT);
    SetTextColor(m_hdc, color);
    RECT rc = rect;
    DrawTextW(m_hdc, text.c_str(), static_cast<int>(text.size()), &rc, format);
    SelectObject(m_hdc, oldFont);
}

} // namespace shell::ui
