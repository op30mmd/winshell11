#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace shell::common {

struct ThemeConfig {
    std::wstring accentColor = L"#2D7DFF";
    int taskbarHeight = 40;
    int iconSize = 48;
};

struct DesktopConfig {
    bool showIcons = true;
    int gridSpacing = 96;
    std::wstring wallpaperPath;
};

struct TaskbarConfig {
    std::wstring position = L"bottom";
    bool showClock = true;
    bool multiMonitor = false;
};

struct LauncherConfig {
    std::vector<std::wstring> pinnedApps;
};

struct HotkeysConfig {
    bool winKeyOpensLauncher = true;
};

struct WatchdogConfig {
    bool restartOnCrash = true;
    int maxRestartsPerMinute = 5;
};

struct Config {
    ThemeConfig theme;
    DesktopConfig desktop;
    TaskbarConfig taskbar;
    LauncherConfig launcher;
    HotkeysConfig hotkeys;
    WatchdogConfig watchdog;

    static Config Load();
    bool Save(const std::filesystem::path& path) const;
};

} // namespace shell::common
