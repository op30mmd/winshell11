#pragma once
#include "taskbar/TaskbarWidgets.h"
#include "ui/UiHost.h"
#include "windowwatcher/WindowWatcher.h"
#include "common/Config.h"
#include <functional>
#include <string>
#include <vector>
#include <windows.h>

namespace shell::flyout {
class FlyoutWindow;
}
namespace shell::tray {
class TrayHost;
}

namespace shell::taskbar {

class TaskbarWindow {
public:
    using StartMenuCallback = std::function<void()>;

    bool Create(HINSTANCE hInstance, const common::TaskbarConfig& config);
    HWND GetHWND() const { return m_hwnd; }
    void SetWindows(const std::vector<watcher::WindowInfo>& windows);
    void SetStartMenuCallback(StartMenuCallback cb) { m_onStartClick = std::move(cb); }
    void SetTrayHost(tray::TrayHost* host);
    void SetFlyout(flyout::FlyoutWindow* flyout) { m_flyout = flyout; }
    void RepositionTrayArea();

private:
    void BuildWidgetTree();
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void RegisterAppBar(bool registerIt);
    void ShowFlyout();
    void ActivateWindow(int index);
    void ShowWindowMenu(HWND hwnd, POINT pt, int windowIndex);
    int GetTrayWidth() const;
    void CheckActiveWindow();

    HWND m_hwnd = nullptr;
    common::TaskbarConfig m_config;
    std::vector<watcher::WindowInfo> m_windows;
    StartMenuCallback m_onStartClick;
    tray::TrayHost* m_trayHost = nullptr;
    flyout::FlyoutWindow* m_flyout = nullptr;

    // New UI framework
    ui::UiHost m_ui;

    // Appbar callback message
    UINT m_appbarMsg = 0;
};

} // namespace shell::taskbar
