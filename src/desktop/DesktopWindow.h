#pragma once
#include "desktop/DesktopWidgets.h"
#include "ui/UiHost.h"
#include "common/Config.h"
#include <string>
#include <vector>
#include <windows.h>
#include <shlobj.h>

namespace shell::desktop {

class DesktopWindow {
public:
    bool Create(HINSTANCE hInstance, const common::DesktopConfig& config);
    ~DesktopWindow();
    HWND GetHWND() const { return m_hwnd; }
    void SetIcons(std::vector<DesktopIconData>& icons);

private:
    void BuildWidgetTree();
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void LaunchItem(int index);
    void ShowItemMenu(HWND hwnd, POINT pt, int index);

    HWND m_hwnd = nullptr;
    common::DesktopConfig m_config;
    ULONG_PTR m_gdiplusToken = 0;

    // New UI framework
    ui::UiHost m_ui;
};

} // namespace shell::desktop
