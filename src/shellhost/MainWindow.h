#pragma once
#include <windows.h>

namespace shell::host {

class MainWindow {
public:
    bool Create(HINSTANCE hInstance);
    HWND GetHWND() const { return m_hwnd; }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HWND m_hwnd = nullptr;
};

} // namespace shell::host
