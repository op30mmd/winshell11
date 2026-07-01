#include "AppLauncher.h"
#include "common/Logger.h"
#include <shellapi.h>
#include <shlobj.h>

namespace shell::launcher {

void AppLauncher::EnumerateApps() {
    m_apps.clear();
}

void AppLauncher::Launch(const AppItem& item) {
    ShellExecuteW(nullptr, L"open", item.path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

} // namespace shell::launcher
