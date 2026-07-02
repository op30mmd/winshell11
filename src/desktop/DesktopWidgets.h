#pragma once
#include "ui/Widget.h"
#include "common/Config.h"
#include <string>
#include <vector>
#include <windows.h>
#include <shlobj.h>

namespace shell::desktop {

struct DesktopIconData {
    std::wstring name;
    std::wstring path;
    int x = 0, y = 0;
    HICON hIcon = nullptr;
    LPITEMIDLIST pidl = nullptr;
    std::wstring parsingName;
};

// ── DesktopIconWidget ─────────────────────────────────────────────────
class DesktopIconWidget : public ui::Widget {
public:
    void SetIconData(const DesktopIconData& data);
    const DesktopIconData& GetIconData() const { return m_data; }

    void Paint(ui::Renderer& renderer, const ui::Theme& theme) override;

    static constexpr int kIconSize = 48;
    static constexpr int kTextHeight = 22;

private:
    DesktopIconData m_data;
};

// ── DesktopPanel (root widget for the desktop window) ─────────────────
class DesktopPanel : public ui::Widget {
public:
    ~DesktopPanel();

    // Wallpaper support (takes ownership of the GDI+ Image pointer)
    void SetWallpaper(void* gdiplusImage) { m_wallpaper = gdiplusImage; }
    void* GetWallpaper() const { return m_wallpaper; }

    // Rebuild icon children from a vector of icon data (takes ownership of handles)
    void SetIcons(std::vector<DesktopIconData>& icons);

    const std::vector<DesktopIconData>& GetIcons() const { return m_icons; }

    void Paint(ui::Renderer& renderer, const ui::Theme& theme) override;
    void Layout() override;

    // Hit test for icons (returns index or -1)
    int HitTestIcon(POINT pt) const;

private:
    void ClearIcons();
    void RebuildChildren();

    void* m_wallpaper = nullptr;
    std::vector<DesktopIconData> m_icons;
};

} // namespace shell::desktop
