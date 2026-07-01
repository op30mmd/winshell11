#pragma once
#include <windows.h>
#include "common/Config.h"

namespace shell::desktop {

class DesktopWindow {
public:
    bool Create(HINSTANCE hInstance, const common::DesktopConfig& config);
    HWND GetHWND() const { return m_hwnd; }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HWND m_hwnd = nullptr;
    common::DesktopConfig m_config;
};

} // namespace shell::desktop
