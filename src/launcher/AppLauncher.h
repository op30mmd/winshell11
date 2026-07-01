#pragma once
#include <string>
#include <vector>
#include <windows.h>

namespace shell::launcher {

struct AppItem {
    std::wstring name;
    std::wstring path;
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
