#pragma once
#include <functional>
#include <windows.h>

namespace shell::host {

class MainWindow {
public:
    using HotkeyHandler = std::function<void(int id)>;

    bool Create(HINSTANCE hInstance);
    HWND GetHWND() const { return m_hwnd; }
    void SetHotkeyHandler(HotkeyHandler handler) { m_hotkeyHandler = std::move(handler); }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HWND m_hwnd = nullptr;
    HotkeyHandler m_hotkeyHandler;
};

} // namespace shell::host
