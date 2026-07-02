#pragma once
#include "Widget.h"
#include <functional>
#include <optional>
#include <string>

namespace shell::ui {

// Static text.
class Label : public Widget {
public:
    void SetText(std::wstring text);
    const std::wstring& GetText() const { return m_text; }
    void SetAlignment(UINT dtFormat) { m_format = dtFormat; }
    void SetSecondary(bool secondary) { m_secondary = secondary; }
    void SetColor(COLORREF color) { m_color = color; }
    void SetFontSize(int size) { m_fontSize = size; }
    void SetBold(bool bold) { m_bold = bold; }

    void Paint(Renderer& renderer, const Theme& theme) override;

private:
    std::wstring m_text;
    UINT m_format = DT_LEFT | DT_VCENTER | DT_SINGLELINE;
    bool m_secondary = false;
    std::optional<COLORREF> m_color;
    int m_fontSize = 0; // 0 = theme default
    bool m_bold = false;
};

// Clickable rounded button with hover/pressed feedback.
class Button : public Widget {
public:
    using ClickHandler = std::function<void()>;

    void SetText(std::wstring text);
    void SetOnClick(ClickHandler handler) { m_onClick = std::move(handler); }

    void Paint(Renderer& renderer, const Theme& theme) override;
    bool OnMouseDown(POINT pt) override;
    bool OnMouseUp(POINT pt) override;

private:
    std::wstring m_text;
    ClickHandler m_onClick;
};

// Horizontal slider with values in [0, 100]. Dragging is handled through
// the UiHost mouse capture, matching the drag behavior in FlyoutWindow.
class Slider : public Widget {
public:
    using ChangeHandler = std::function<void(int)>;

    void SetValue(int value);
    int GetValue() const { return m_value; }
    void SetOnChange(ChangeHandler handler) { m_onChange = std::move(handler); }

    void Paint(Renderer& renderer, const Theme& theme) override;
    bool OnMouseDown(POINT pt) override;
    bool OnMouseMove(POINT pt) override;

private:
    void UpdateValueFromPoint(POINT pt);

    static constexpr int kThumbRadius = 8;
    static constexpr int kTrackHeight = 4;

    int m_value = 0;
    ChangeHandler m_onChange;
};

enum class Orientation { Horizontal, Vertical };

// Lays out visible children sequentially using their preferred sizes.
// A preferred size of 0 on the cross axis stretches the child; a preferred
// size of 0 on the main axis falls back to a 32px default.
class StackPanel : public Widget {
public:
    void SetOrientation(Orientation orientation) { m_orientation = orientation; }
    void SetSpacing(int spacing) { m_spacing = spacing; }
    void SetPadding(int padding) { m_padding = padding; }
    void SetBackground(COLORREF color) { m_background = color; }

    void Paint(Renderer& renderer, const Theme& theme) override;
    void Layout() override;

private:
    Orientation m_orientation = Orientation::Vertical;
    int m_spacing = 4;
    int m_padding = 0;
    std::optional<COLORREF> m_background;
};

} // namespace shell::ui
