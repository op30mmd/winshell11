#pragma once
#include <windows.h>
#include <vector>
#include <string>

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

private:
    std::vector<AppItem> m_apps;
};

} // namespace shell::launcher
