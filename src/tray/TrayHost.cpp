#include "TrayHost.h"
#include "common/Logger.h"
#include <shellapi.h>

namespace shell::tray {

#define WM_TRAY_CALLBACK (WM_USER + 100)

bool TrayHost::Initialize(HWND hTaskbar) {
    const wchar_t CLASS_NAME[] = L"Shell_TrayWnd";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = CLASS_NAME;

    RegisterClassW(&wc);

    m_hwnd = CreateWindowExW(
        0, CLASS_NAME, nullptr,
        WS_CHILD, 0, 0, 0, 0,
        hTaskbar, nullptr, wc.hInstance, this
    );

    if (m_hwnd) {
        UINT msg = RegisterWindowMessageW(L"TaskbarCreated");
        PostMessage(HWND_BROADCAST, msg, 0, 0);
    }

    return m_hwnd != nullptr;
}

void TrayHost::Shutdown() {
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

LRESULT CALLBACK TrayHost::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    TrayHost* pThis = nullptr;
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (TrayHost*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (TrayHost*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (pThis) {
        // Handling NIM_ADD, NIM_MODIFY, NIM_DELETE
        // In a real shell, these are often delivered via a private message or WM_COPYDATA
        // that wraps the NOTIFYICONDATA structure.
        switch (uMsg) {
            case WM_COPYDATA: {
                PCOPYDATASTRUCT pcds = (PCOPYDATASTRUCT)lParam;
                // De-serialize NOTIFYICONDATA and update m_icons
                return TRUE;
            }
        }
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

} // namespace shell::tray
