#pragma once
#include <windows.h>
#include "common/Config.h"

namespace shell::taskbar {

class TaskbarWindow {
public:
    bool Create(HINSTANCE hInstance, const common::TaskbarConfig& config);
    HWND GetHWND() const { return m_hwnd; }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void RegisterAppBar(bool registerIt);

    HWND m_hwnd = nullptr;
    common::TaskbarConfig m_config;
};

} // namespace shell::taskbar
