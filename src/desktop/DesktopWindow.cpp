#include "DesktopWindow.h"
#include "common/Logger.h"
#include <gdiplus.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <windowsx.h>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "Shlwapi.lib")

namespace shell::desktop {

using namespace Gdiplus;

// GDI+ wrapper for image loading (used during window creation)
static Image* LoadWallpaperImage(const std::wstring& path) {
    auto* img = Image::FromFile(path.c_str(), FALSE);
    if (img && img->GetLastStatus() == Ok)
        return img;
    delete img;
    return nullptr;
}

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

void DesktopWindow::ShowItemMenu(HWND hwnd, POINT pt, int index) {
    auto* panel = static_cast<DesktopPanel*>(m_ui.GetRoot());
    if (!panel) return;
    const auto& icons = panel->GetIcons();
    if (index < 0 || index >= (int)icons.size())
        return;

    LPITEMIDLIST pidlAbs = icons[index].pidl;
    if (!pidlAbs)
        return;

    IShellFolder* pDesktopFolder = nullptr;
    if (FAILED(SHGetDesktopFolder(&pDesktopFolder)))
        return;

    LPCITEMIDLIST pidlChild = ILFindLastID(pidlAbs);
    if (!pidlChild) {
        pDesktopFolder->Release();
        return;
    }

    IContextMenu* pContextMenu = nullptr;
    PCUITEMID_CHILD pidlChildren[1] = {pidlChild};
    HRESULT hr = pDesktopFolder->GetUIObjectOf(hwnd, 1, pidlChildren, IID_IContextMenu, nullptr, (void**)&pContextMenu);
    if (SUCCEEDED(hr)) {
        HMENU hMenu = CreatePopupMenu();
        if (hMenu) {
            hr = pContextMenu->QueryContextMenu(hMenu, 0, 1, 0x7FFF, CMF_NORMAL | CMF_EXTENDEDVERBS | CMF_EXPLORE);
            if (SUCCEEDED(hr)) {
                UINT cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
                if (cmd > 0) {
                    CMINVOKECOMMANDINFOEX ici = {sizeof(ici)};
                    ici.cbSize = sizeof(CMINVOKECOMMANDINFOEX);
                    ici.hwnd = hwnd;
                    ici.lpVerb = MAKEINTRESOURCEA(cmd - 1);
                    ici.nShow = SW_SHOWNORMAL;
                    ici.lpDirectoryW = L"C:\\";
                    ici.fMask = CMIC_MASK_UNICODE;
                    pContextMenu->InvokeCommand((CMINVOKECOMMANDINFO*)&ici);
                }
            }
            DestroyMenu(hMenu);
        }
        pContextMenu->Release();
    }
    pDesktopFolder->Release();
}

void DesktopWindow::LaunchItem(int index) {
    auto* panel = static_cast<DesktopPanel*>(m_ui.GetRoot());
    if (!panel) return;
    const auto& icons = panel->GetIcons();
    if (index < 0 || index >= (int)icons.size())
        return;

    const DesktopIconData& icon = icons[index];

    // Try filesystem path first
    if (!icon.path.empty()) {
        ShellExecuteW(m_hwnd, L"open", icon.path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return;
    }

    // Try parsing name (works for many virtual items like Recycle Bin)
    if (!icon.parsingName.empty()) {
        ShellExecuteW(m_hwnd, L"open", icon.parsingName.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return;
    }

    // PIDL-based launch
    if (icon.pidl) {
        SHELLEXECUTEINFOW sei = {sizeof(sei)};
        sei.hwnd = m_hwnd;
        sei.lpIDList = icon.pidl;
        sei.lpVerb = L"open";
        sei.fMask = SEE_MASK_IDLIST | SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC;
        sei.nShow = SW_SHOWNORMAL;
        ShellExecuteExW(&sei);
    }
}

void DesktopWindow::BuildWidgetTree() {
    auto panel = std::make_unique<DesktopPanel>();

    // Try to load wallpaper
    if (!m_config.wallpaperPath.empty()) {
        Image* img = LoadWallpaperImage(m_config.wallpaperPath);
        if (img)
            panel->SetWallpaper(img);
    }

    m_ui.SetRoot(std::move(panel));
}

bool DesktopWindow::Create(HINSTANCE hInstance, const common::DesktopConfig& config) {
    m_config = config;

    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr);

    const wchar_t CLASS_NAME[] = L"ShellDesktopWindowClass";
    WNDCLASSW wc = {};
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));

    RegisterClassW(&wc);

    m_hwnd = CreateWindowExW(WS_EX_TOOLWINDOW, CLASS_NAME, L"Desktop", WS_POPUP | WS_VISIBLE, 0, 0,
                             GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), nullptr, nullptr, hInstance,
                             this);

    if (m_hwnd) {
        SetWindowPos(m_hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        m_ui.Attach(m_hwnd);
        m_ui.GetTheme().background = RGB(30, 30, 36);
        BuildWidgetTree();
    }

    return m_hwnd != nullptr;
}

DesktopWindow::~DesktopWindow() {
    if (m_gdiplusToken)
        GdiplusShutdown(m_gdiplusToken);
}

void DesktopWindow::SetIcons(std::vector<DesktopIconData>& icons) {
    auto* panel = static_cast<DesktopPanel*>(m_ui.GetRoot());
    if (panel)
        panel->SetIcons(icons);
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
        // Route paint and input through the UI host (once attached).
        LRESULT result = 0;
        if (pThis->m_ui.IsAttached() && pThis->m_ui.HandleMessage(uMsg, wParam, lParam, result))
            return result;

        switch (uMsg) {
            case WM_LBUTTONDBLCLK: {
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                auto* panel = static_cast<DesktopPanel*>(pThis->m_ui.GetRoot());
                if (panel) {
                    int hit = panel->HitTestIcon(pt);
                    if (hit >= 0)
                        pThis->LaunchItem(hit);
                }
                return 0;
            }

            case WM_CONTEXTMENU: {
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                auto* panel = static_cast<DesktopPanel*>(pThis->m_ui.GetRoot());
                if (panel) {
                    int hit = panel->HitTestIcon(pt);
                    if (hit >= 0)
                        pThis->ShowItemMenu(hwnd, pt, hit);
                    else
                        ShowDesktopContextMenu(hwnd, pt);
                }
                return 0;
            }

            case WM_SYSCOMMAND:
                if (wParam == SC_MINIMIZE || wParam == SC_CLOSE || wParam == SC_SCREENSAVE)
                    return 0;
                return DefWindowProcW(hwnd, uMsg, wParam, lParam);

            case WM_CLOSE:
                return 0;
        }
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

} // namespace shell::desktop
