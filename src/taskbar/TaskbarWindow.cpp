#include "TaskbarWindow.h"
#include "common/Logger.h"
#include <shellapi.h>

namespace shell::taskbar {

bool TaskbarWindow::Create(HINSTANCE hInstance, const common::TaskbarConfig& config) {
    m_config = config;
    const wchar_t CLASS_NAME[] = L"ShellTaskbarWindowClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassW(&wc);

    int height = 40;
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    m_hwnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, CLASS_NAME, L"Taskbar", WS_POPUP | WS_VISIBLE, 0,
                             screenHeight - height, screenWidth, height, nullptr, nullptr, hInstance, this);

    if (m_hwnd) {
        RegisterAppBar(true);
    }

    return m_hwnd != nullptr;
}

void TaskbarWindow::RegisterAppBar(bool registerIt) {
    APPBARDATA abd = {sizeof(abd), m_hwnd};
    if (registerIt) {
        SHAppBarMessage(ABM_NEW, &abd);
        abd.uEdge = ABE_BOTTOM;
        abd.rc.left = 0;
        abd.rc.top = GetSystemMetrics(SM_CYSCREEN) - 40;
        abd.rc.right = GetSystemMetrics(SM_CXSCREEN);
        abd.rc.bottom = GetSystemMetrics(SM_CYSCREEN);
        SHAppBarMessage(ABM_SETPOS, &abd);
    } else {
        SHAppBarMessage(ABM_REMOVE, &abd);
    }
}

LRESULT CALLBACK TaskbarWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    TaskbarWindow* pThis = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (TaskbarWindow*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (TaskbarWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (pThis) {
        switch (uMsg) {
            case WM_DESTROY:
                pThis->RegisterAppBar(false);
                return 0;
        }
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

} // namespace shell::taskbar
