#include "DesktopWindow.h"
#include "common/Logger.h"
#include <shlobj.h>
#include <windowsx.h>

namespace shell::desktop {

static void ShowDesktopContextMenu(HWND hwnd, POINT pt) {
    IShellFolder* pDesktopFolder = nullptr;
    if (SUCCEEDED(SHGetDesktopFolder(&pDesktopFolder))) {
        IContextMenu* pContextMenu = nullptr;
        HRESULT hr = pDesktopFolder->CreateViewObject(hwnd, IID_IContextMenu, (void**)&pContextMenu);
        if (SUCCEEDED(hr)) {
            HMENU hMenu = CreatePopupMenu();
            if (hMenu) {
                hr = pContextMenu->QueryContextMenu(hMenu, 0, 1, 0x7FFF, CMF_NORMAL | CMF_EXTENDEDVERBS);
                if (SUCCEEDED(hr)) {
                    UINT cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
                    if (cmd > 0) {
                        CMINVOKECOMMANDINFO ici = {sizeof(ici)};
                        ici.lpVerb = MAKEINTRESOURCEA(cmd - 1);
                        ici.nShow = SW_SHOWNORMAL;
                        pContextMenu->InvokeCommand(&ici);
                    }
                }
                DestroyMenu(hMenu);
            }
            pContextMenu->Release();
        }
        pDesktopFolder->Release();
    }
}

bool DesktopWindow::Create(HINSTANCE hInstance, const common::DesktopConfig& config) {
    m_config = config;
    const wchar_t CLASS_NAME[] = L"ShellDesktopWindowClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);

    RegisterClassW(&wc);

    m_hwnd = CreateWindowExW(WS_EX_TOOLWINDOW, CLASS_NAME, L"Desktop", WS_POPUP | WS_VISIBLE, 0, 0,
                             GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), nullptr, nullptr, hInstance,
                             this);

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
                BeginPaint(hwnd, &ps);
                EndPaint(hwnd, &ps);
                return 0;
            }
            case WM_CONTEXTMENU: {
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                ShowDesktopContextMenu(hwnd, pt);
                return 0;
            }
        }
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

} // namespace shell::desktop
