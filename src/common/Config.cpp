#include "Config.h"
#include <fstream>
#include <shlobj.h>
#include <windows.h>

namespace shell::common {

static std::wstring ReadWallpaperFromRegistry() {
    HKEY hKey;
    std::wstring path;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
        wchar_t buffer[1024] = {};
        DWORD size = sizeof(buffer);
        if (RegQueryValueExW(hKey, L"Wallpaper", nullptr, nullptr, (LPBYTE)buffer, &size) == ERROR_SUCCESS) {
            path = buffer;
        }
        RegCloseKey(hKey);
    }
    return path;
}

Config Config::Load() {
    Config config;
    // Read the current Windows wallpaper from the registry so the desktop
    // shows the actual wallpaper instead of a solid dark color.
    config.desktop.wallpaperPath = ReadWallpaperFromRegistry();
    return config;
}

bool Config::Save(const std::filesystem::path& /*path*/) const {
    return false;
}

} // namespace shell::common
