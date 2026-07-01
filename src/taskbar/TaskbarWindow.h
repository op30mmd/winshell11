#pragma once
#include "windowwatcher/WindowWatcher.h"
#include "common/Config.h"
#include <functional>
#include <string>
#include <vector>
#include <windows.h>

namespace shell::taskbar {

class TaskbarWindow {
public:
    using StartMenuCallback = std::function<void()>;

    bool Create(HINSTANCE hInstance, const common::TaskbarConfig& config);
    HWND GetHWND() const { return m_hwnd; }
    void SetWindows(const std::vector<watcher::WindowInfo>& windows);
    void SetStartMenuCallback(StartMenuCallback cb) { m_onStartClick = std::move(cb); }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void RegisterAppBar(bool registerIt);
    void DrawStartButton(HDC hdc, int height);
    void DrawClock(HDC hdc, int width, int height);
    void DrawButtons(HDC hdc, int startX, int endX, int height);
    int HitTest(POINT pt) const;
    void ActivateWindow(int index);
    std::wstring GetTimeString() const;

    HWND m_hwnd = nullptr;
    common::TaskbarConfig m_config;
    std::vector<watcher::WindowInfo> m_windows;
    StartMenuCallback m_onStartClick;
    int m_buttonWidth = 160;
    int m_startBtnWidth = 48;
    int m_clockWidth = 100;
};

} // namespace shell::taskbar
