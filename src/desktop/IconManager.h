#pragma once
#include <string>
#include <vector>
#include <windows.h>
#include <shlobj.h>

namespace shell::desktop {

struct DesktopIcon {
    std::wstring name;
    std::wstring path;
    std::wstring parsingName;
    LPITEMIDLIST pidl = nullptr;
    int x = 0, y = 0;
};

class IconManager {
public:
    void EnumerateIcons();
    const std::vector<DesktopIcon>& GetIcons() const { return m_icons; }
    void FreeIcons();

private:
    std::vector<DesktopIcon> m_icons;
};

} // namespace shell::desktop
