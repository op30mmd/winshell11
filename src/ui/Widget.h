#pragma once
#include <memory>
#include <vector>
#include <windows.h>

namespace shell::ui {

class Renderer;
class UiHost;
struct Theme;

// Base class for all UI elements. Bounds are expressed in window client
// coordinates. Widgets form a tree; the root is owned by a UiHost, which
// dispatches paint and input messages into the tree.
class Widget {
public:
    virtual ~Widget() = default;

    void SetBounds(const RECT& bounds) { m_bounds = bounds; }
    const RECT& GetBounds() const { return m_bounds; }
    int Width() const { return m_bounds.right - m_bounds.left; }
    int Height() const { return m_bounds.bottom - m_bounds.top; }

    // Preferred size is consumed by layout containers (see StackPanel).
    // A value of 0 means "stretch to fill the available space".
    void SetPreferredSize(int w, int h) {
        m_prefW = w;
        m_prefH = h;
    }
    int PreferredWidth() const { return m_prefW; }
    int PreferredHeight() const { return m_prefH; }

    void SetVisible(bool visible);
    bool IsVisible() const { return m_visible; }

    Widget* AddChild(std::unique_ptr<Widget> child);
    const std::vector<std::unique_ptr<Widget>>& Children() const { return m_children; }
    Widget* GetParent() const { return m_parent; }

    // Painting and layout.
    virtual void Paint(Renderer& renderer, const Theme& theme);
    virtual void Layout();

    // Returns the deepest visible widget containing pt, or nullptr.
    virtual Widget* HitTest(POINT pt);

    // Input events. Return true when the event is handled.
    virtual bool OnMouseDown(POINT) { return false; }
    virtual bool OnMouseUp(POINT) { return false; }
    virtual bool OnMouseMove(POINT) { return false; }
    virtual void OnMouseEnter() {}
    virtual void OnMouseLeave() {}

    bool IsHovered() const { return m_hovered; }
    bool IsPressed() const { return m_pressed; }

    // Requests a repaint of the host window.
    void Invalidate();
    UiHost* GetHost() const { return m_host; }

    // Internal: used by UiHost to manage tree state.
    void SetHostRecursive(UiHost* host);
    void SetHoverState(bool hovered);
    void SetPressedState(bool pressed);

protected:
    void PaintChildren(Renderer& renderer, const Theme& theme);

private:
    RECT m_bounds = {0, 0, 0, 0};
    int m_prefW = 0;
    int m_prefH = 0;
    bool m_visible = true;
    bool m_hovered = false;
    bool m_pressed = false;
    UiHost* m_host = nullptr;
    Widget* m_parent = nullptr;
    std::vector<std::unique_ptr<Widget>> m_children;
};

} // namespace shell::ui
