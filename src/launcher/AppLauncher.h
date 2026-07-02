#pragma once
#include <string>
#include <vector>
#include <windows.h>

namespace shell::launcher {

enum class AppType : int {
    Application  = 0,
    ControlPanel = 1,
    SystemTool   = 2,
    WebLink      = 3,
    Unknown      = 4
};

struct AppItem {
    std::wstring name;
    std::wstring path;      // .lnk path
    std::wstring target;    // resolved shortcut target
    int type;               // AppType as int
};

class AppLauncher {
public:
    void EnumerateApps();
    const std::vector<AppItem>& GetApps() const { return m_apps; }
    void Launch(const AppItem& item);
    bool ShowMenu(HWND hParent);

private:
    std::vector<AppItem> m_apps;
};

} // namespace shell::launcher
