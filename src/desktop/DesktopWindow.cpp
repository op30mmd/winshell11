#include "DesktopWindow.h"
#include "common/Logger.h"

namespace shell::desktop {

bool DesktopWindow::Create(HINSTANCE hInstance, const common::DesktopConfig& config) {
    m_config = config;
    const wchar_t CLASS_NAME[] = L"ShellDesktopWindowClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);

    RegisterClassW(&wc);

    m_hwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW, CLASS_NAME, L"Desktop",
        WS_POPUP | WS_VISIBLE,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        nullptr, nullptr, hInstance, this
    );

    if (m_hwnd) {
        SetWindowPos(m_hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    return m_hwnd != nullptr;
}

LRESULT CALLBACK DesktopWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    DesktopWindow* pThis = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (DesktopWindow*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (DesktopWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (pThis) {
        switch (uMsg) {
            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                EndPaint(hwnd, &ps);
                return 0;
            }
        }
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

} // namespace shell::desktop
