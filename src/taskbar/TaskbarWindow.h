#pragma once
#include "windowwatcher/WindowWatcher.h"
#include "common/Config.h"
#include <functional>
#include <string>
#include <vector>
#include <windows.h>

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
    void SetTrayHost(tray::TrayHost* host) { m_trayHost = host; }
    void RepositionTrayArea();

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void RegisterAppBar(bool registerIt);
    void DrawStartButton(HDC hdc, int height);
    void DrawClock(HDC hdc, int width, int height);
    void DrawButtons(HDC hdc, int startX, int endX, int height);
    void DrawTrayIcons(HDC hdc, int width, int height);
    int HitTest(POINT pt) const;
    void ActivateWindow(int index);
    void ShowWindowMenu(HWND hwnd, POINT pt, int windowIndex);
    std::wstring GetTimeString() const;
    int GetTrayWidth() const;
    void CheckActiveWindow();
    int GetActiveIndex() const;
    void EnsureBuffer(int w, int h);
    void DestroyBuffer();

    HWND m_hwnd = nullptr;
    common::TaskbarConfig m_config;
    std::vector<watcher::WindowInfo> m_windows;
    StartMenuCallback m_onStartClick;
    tray::TrayHost* m_trayHost = nullptr;
    int m_buttonWidth = 160;
    int m_startBtnWidth = 48;
    int m_clockWidth = 100;

    // Active-window transition animation
    int m_prevActiveIdx = -1;
    int m_nextActiveIdx = -1;
    ULONGLONG m_animStartTick = 0;
    static constexpr int ANIM_DURATION_MS = 180;

    // Hover tracking
    int m_hoveredIdx = -1;

    // Appbar callback message
    UINT m_appbarMsg = 0;

    // Cached double-buffer
    HDC m_cacheDC = nullptr;
    HBITMAP m_cacheBmp = nullptr;
    HBITMAP m_cacheOldBmp = nullptr;
    int m_cacheW = 0;
    int m_cacheH = 0;
};

} // namespace shell::taskbar
