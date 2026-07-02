#include "Controls.h"
#include "Renderer.h"
#include "Theme.h"

namespace shell::ui {

// --- Label ---------------------------------------------------------------

void Label::SetText(std::wstring text) {
    if (m_text != text) {
        m_text = std::move(text);
        Invalidate();
    }
}

void Label::Paint(Renderer& renderer, const Theme& theme) {
    COLORREF color = m_color.value_or(m_secondary ? theme.textSecondary : theme.text);
    const int size = m_fontSize > 0 ? m_fontSize : theme.fontSize;
    renderer.DrawString(m_text, GetBounds(), color, m_format | DT_NOPREFIX, theme.fontFamily, size, m_bold);
    PaintChildren(renderer, theme);
}

// --- Button --------------------------------------------------------------

void Button::SetText(std::wstring text) {
    if (m_text != text) {
        m_text = std::move(text);
        Invalidate();
    }
}

void Button::Paint(Renderer& renderer, const Theme& theme) {
    COLORREF bg = theme.surface;
    if (IsPressed())
        bg = theme.surfacePressed;
    else if (IsHovered())
        bg = theme.surfaceHover;

    renderer.FillRoundedRect(GetBounds(), bg, theme.cornerRadius);
    renderer.DrawString(m_text, GetBounds(), theme.text, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX,
                        theme.fontFamily, theme.fontSize, false);
    PaintChildren(renderer, theme);
}

bool Button::OnMouseDown(POINT) {
    return true;
}

bool Button::OnMouseUp(POINT pt) {
    // Fire only when the button is released over the widget, so users can
    // cancel a click by dragging off before releasing.
    if (PtInRect(&GetBounds(), pt) && m_onClick)
        m_onClick();
    return true;
}

// --- Slider --------------------------------------------------------------

void Slider::SetValue(int value) {
    const int clamped = value < 0 ? 0 : (value > 100 ? 100 : value);
    if (m_value != clamped) {
        m_value = clamped;
        Invalidate();
    }
}

void Slider::Paint(Renderer& renderer, const Theme& theme) {
    const RECT& b = GetBounds();
    const int cy = (b.top + b.bottom) / 2;

    RECT track = {b.left, cy - kTrackHeight / 2, b.right, cy + kTrackHeight / 2};
    renderer.FillRoundedRect(track, theme.border, kTrackHeight / 2);

    const int fillX = b.left + MulDiv(b.right - b.left, m_value, 100);
    if (fillX > b.left) {
        RECT fill = {b.left, track.top, fillX, track.bottom};
        renderer.FillRoundedRect(fill, theme.accent, kTrackHeight / 2);
    }

    RECT thumb = {fillX - kThumbRadius, cy - kThumbRadius, fillX + kThumbRadius, cy + kThumbRadius};
    renderer.FillEllipse(thumb, (IsHovered() || IsPressed()) ? theme.accent : theme.text);

    PaintChildren(renderer, theme);
}

bool Slider::OnMouseDown(POINT pt) {
    UpdateValueFromPoint(pt);
    return true;
}

bool Slider::OnMouseMove(POINT pt) {
    if (IsPressed()) {
        UpdateValueFromPoint(pt);
        return true;
    }
    return false;
}

void Slider::UpdateValueFromPoint(POINT pt) {
    const RECT& b = GetBounds();
    const int width = b.right - b.left;
    if (width <= 0)
        return;
    int value = MulDiv(static_cast<int>(pt.x) - b.left, 100, width);
    value = value < 0 ? 0 : (value > 100 ? 100 : value);
    if (value != m_value) {
        m_value = value;
        if (m_onChange)
            m_onChange(m_value);
        Invalidate();
    }
}

// --- StackPanel ------------------------------------------------------------

void StackPanel::Paint(Renderer& renderer, const Theme& theme) {
    if (m_background)
        renderer.FillRoundedRect(GetBounds(), *m_background, theme.cornerRadius);
    PaintChildren(renderer, theme);
}

void StackPanel::Layout() {
    const RECT& b = GetBounds();
    int x = b.left + m_padding;
    int y = b.top + m_padding;

    for (const auto& child : Children()) {
        if (!child->IsVisible())
            continue;

        int w = child->PreferredWidth();
        int h = child->PreferredHeight();

        if (m_orientation == Orientation::Vertical) {
            if (w <= 0)
                w = (b.right - b.left) - m_padding * 2;
            if (h <= 0)
                h = 32;
            child->SetBounds({x, y, x + w, y + h});
            y += h + m_spacing;
        } else {
            if (h <= 0)
                h = (b.bottom - b.top) - m_padding * 2;
            if (w <= 0)
                w = 32;
            child->SetBounds({x, y, x + w, y + h});
            x += w + m_spacing;
        }
        child->Layout();
    }
}

} // namespace shell::ui
