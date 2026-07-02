#include "DesktopWidgets.h"
#include "common/Logger.h"
#include "ui/Renderer.h"
#include "ui/Theme.h"
#include "ui/UiHost.h"
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

namespace shell::desktop {

using namespace Gdiplus;

// ═══════════════════════════════════════════════════════════════════════
//  DesktopIconWidget
// ═══════════════════════════════════════════════════════════════════════

void DesktopIconWidget::SetIconData(const DesktopIconData& data) {
    m_data = data;
}

void DesktopIconWidget::Paint(ui::Renderer& renderer, const ui::Theme& theme) {
    const RECT& b = GetBounds();
    const DesktopIconData& icon = m_data;

    if (icon.hIcon) {
        DrawIconEx(renderer.Handle(), b.left, b.top, icon.hIcon, kIconSize, kIconSize, 0, nullptr, DI_NORMAL);
    }

    RECT textRc = {b.left, b.top + kIconSize, b.right, b.top + kIconSize + kTextHeight};
    renderer.DrawString(icon.name, textRc, RGB(220, 220, 220),
                        DT_CENTER | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX,
                        theme.fontFamily, 14);
}

// ═══════════════════════════════════════════════════════════════════════
//  DesktopPanel
// ═══════════════════════════════════════════════════════════════════════

DesktopPanel::~DesktopPanel() {
    delete reinterpret_cast<Image*>(m_wallpaper);
    ClearIcons();
}

void DesktopPanel::ClearIcons() {
    for (auto& icon : m_icons) {
        if (icon.hIcon)
            DestroyIcon(icon.hIcon);
        if (icon.pidl)
            CoTaskMemFree(icon.pidl);
    }
    m_icons.clear();
}

void DesktopPanel::SetIcons(std::vector<DesktopIconData>& icons) {
    ClearIcons();
    m_icons = std::move(icons);
    LOG_INFO("DesktopPanel: SetIcons with %zu icons", m_icons.size());
    RebuildChildren();
    if (GetHost())
        GetHost()->Invalidate();
}

void DesktopPanel::RebuildChildren() {
    ClearChildren();

    for (auto& iconData : m_icons) {
        auto iconWidget = std::make_unique<DesktopIconWidget>();
        iconWidget->SetIconData(iconData);
        AddChild(std::move(iconWidget));
    }

    LOG_INFO("DesktopPanel: rebuilt %zu children, m_icons=%zu", Children().size(), m_icons.size());

    if (GetHost())
        GetHost()->PerformLayout();
}

void DesktopPanel::Paint(ui::Renderer& renderer, const ui::Theme& theme) {
    RECT rc = GetBounds();
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    if (m_wallpaper) {
        auto* img = reinterpret_cast<Image*>(m_wallpaper);
        Graphics graphics(renderer.Handle());
        graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        graphics.DrawImage(img, 0, 0, w, h);
    } else {
        renderer.FillRect(rc, RGB(30, 30, 36));
    }

    Widget::Paint(renderer, theme);
}

void DesktopPanel::Layout() {
    // Position each icon child at its stored x,y
    for (size_t i = 0; i < Children().size() && i < m_icons.size(); i++) {
        const auto& iconData = m_icons[i];
        auto* iconWidget = static_cast<DesktopIconWidget*>(Children()[i].get());
        int iconW = DesktopIconWidget::kIconSize + 32;   // 48 + 16 margin each side
        int iconH = DesktopIconWidget::kIconSize + DesktopIconWidget::kTextHeight + 4;
        iconWidget->SetBounds({iconData.x, iconData.y, iconData.x + iconW, iconData.y + iconH});
        iconWidget->Layout();
    }
}

int DesktopPanel::HitTestIcon(POINT pt) const {
    for (size_t i = 0; i < Children().size(); i++) {
        const RECT& b = Children()[i]->GetBounds();
        if (PtInRect(&b, pt))
            return (int)i;
    }
    return -1;
}

} // namespace shell::desktop
