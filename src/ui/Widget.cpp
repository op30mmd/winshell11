#include "Widget.h"
#include "Renderer.h"
#include "Theme.h"
#include "UiHost.h"

namespace shell::ui {

void Widget::SetVisible(bool visible) {
    if (m_visible != visible) {
        m_visible = visible;
        Invalidate();
    }
}

Widget* Widget::AddChild(std::unique_ptr<Widget> child) {
    child->m_parent = this;
    child->SetHostRecursive(m_host);
    m_children.push_back(std::move(child));
    return m_children.back().get();
}

void Widget::Paint(Renderer& renderer, const Theme& theme) {
    PaintChildren(renderer, theme);
}

void Widget::PaintChildren(Renderer& renderer, const Theme& theme) {
    for (const auto& child : m_children) {
        if (child->IsVisible())
            child->Paint(renderer, theme);
    }
}

void Widget::Layout() {
    for (const auto& child : m_children)
        child->Layout();
}

Widget* Widget::HitTest(POINT pt) {
    if (!m_visible || !PtInRect(&m_bounds, pt))
        return nullptr;
    // Children painted last are visually on top, so hit test in reverse.
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if (Widget* hit = (*it)->HitTest(pt))
            return hit;
    }
    return this;
}

void Widget::Invalidate() {
    if (m_host)
        m_host->Invalidate();
}

void Widget::SetHostRecursive(UiHost* host) {
    m_host = host;
    for (const auto& child : m_children)
        child->SetHostRecursive(host);
}

void Widget::SetHoverState(bool hovered) {
    if (m_hovered != hovered) {
        m_hovered = hovered;
        if (hovered)
            OnMouseEnter();
        else
            OnMouseLeave();
        Invalidate();
    }
}

void Widget::SetPressedState(bool pressed) {
    if (m_pressed != pressed) {
        m_pressed = pressed;
        Invalidate();
    }
}

} // namespace shell::ui
