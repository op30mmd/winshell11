#pragma once
#include <windows.h>
#include <vector>
#include <string>

namespace shell::desktop {

struct DesktopIcon {
    std::wstring name;
    std::wstring path;
    int x, y;
};

class IconManager {
public:
    void EnumerateIcons();
    const std::vector<DesktopIcon>& GetIcons() const { return m_icons; }

private:
    std::vector<DesktopIcon> m_icons;
};

} // namespace shell::desktop
