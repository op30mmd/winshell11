#pragma once
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

class DesktopWindow {
public:
    bool Create(HINSTANCE hInstance, const common::DesktopConfig& config);
    ~DesktopWindow();
    HWND GetHWND() const { return m_hwnd; }
    void SetIcons(std::vector<DesktopIconData>& icons);

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void DrawWallpaper(HDC hdc);
    void DrawIcons(HDC hdc);
    int HitTestIcon(POINT pt) const;
    void ClearIcons();
    void ShowItemMenu(HWND hwnd, POINT pt, int index);
    void LaunchItem(int index);

    HWND m_hwnd = nullptr;
    common::DesktopConfig m_config;
    std::vector<DesktopIconData> m_icons;
    ULONG_PTR m_gdiplusToken = 0;
    void* m_wallpaper = nullptr;
};

} // namespace shell::desktop
