#include "AppLauncher.h"
#include <shlobj.h>
#include <shellapi.h>
#include "common/Logger.h"

namespace shell::launcher {

void AppLauncher::EnumerateApps() {
    m_apps.clear();
}

void AppLauncher::Launch(const AppItem& item) {
    ShellExecuteW(nullptr, L"open", item.path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

} // namespace shell::launcher
