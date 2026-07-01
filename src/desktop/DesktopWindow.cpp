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

// GDI+ wrapper for image loading
class GdiPlusImage {
public:
    GdiPlusImage(const std::wstring& path) {
        m_image = Image::FromFile(path.c_str(), FALSE);
    }
    ~GdiPlusImage() { delete m_image; }
    bool IsValid() const { return m_image && m_image->GetLastStatus() == Ok; }
    Image* Get() const { return m_image; }

private:
    Image* m_image = nullptr;
};

void DesktopWindow::SetIcons(std::vector<DesktopIconData>& icons) {
    ClearIcons();
    // Take ownership of PIDL pointers from the source
    m_icons = std::move(icons);
    // Null out the source so they don't free them
    for (auto& icon : icons) {
        icon.pidl = nullptr;
        icon.hIcon = nullptr;
    }
    if (m_hwnd)
        InvalidateRect(m_hwnd, nullptr, TRUE);
}

void DesktopWindow::ClearIcons() {
    for (auto& icon : m_icons) {
        if (icon.hIcon)
            DestroyIcon(icon.hIcon);
        if (icon.pidl)
            CoTaskMemFree(icon.pidl);
    }
    m_icons.clear();
}

DesktopWindow::~DesktopWindow() {
    ClearIcons();
    delete reinterpret_cast<GdiPlusImage*>(m_wallpaper);
    if (m_gdiplusToken) {
        GdiplusShutdown(m_gdiplusToken);
    }
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
    if (index < 0 || index >= (int)m_icons.size())
        return;

    LPITEMIDLIST pidlAbs = m_icons[index].pidl;
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
    if (index < 0 || index >= (int)m_icons.size())
        return;

    const DesktopIconData& icon = m_icons[index];

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

bool DesktopWindow::Create(HINSTANCE hInstance, const common::DesktopConfig& config) {
    m_config = config;

    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr);

    if (!config.wallpaperPath.empty()) {
        auto* img = new GdiPlusImage(config.wallpaperPath);
        if (img->IsValid()) {
            m_wallpaper = img;
        } else {
            delete img;
            m_wallpaper = nullptr;
        }
    }

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
    }

    return m_hwnd != nullptr;
}

void DesktopWindow::DrawWallpaper(HDC hdc) {
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    if (m_wallpaper) {
        auto* img = reinterpret_cast<GdiPlusImage*>(m_wallpaper);
        Graphics graphics(hdc);
        graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        graphics.DrawImage(img->Get(), 0, 0, w, h);
    } else {
        HBRUSH hBrush = CreateSolidBrush(RGB(30, 30, 36));
        FillRect(hdc, &rc, hBrush);
        DeleteObject(hBrush);
    }
}

void DesktopWindow::DrawIcons(HDC hdc) {
    for (const auto& icon : m_icons) {
        if (icon.hIcon) {
            DrawIconEx(hdc, icon.x, icon.y, icon.hIcon, 48, 48, 0, nullptr, DI_NORMAL);
        }

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(220, 220, 220));
        HFONT hFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                   CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

        RECT textRc = {icon.x, icon.y + 50, icon.x + 80, icon.y + 72};
        DrawTextW(hdc, icon.name.c_str(), -1, &textRc, DT_CENTER | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
    }
}

int DesktopWindow::HitTestIcon(POINT pt) const {
    for (size_t i = 0; i < m_icons.size(); i++) {
        RECT iconRc = {m_icons[i].x, m_icons[i].y, m_icons[i].x + 80, m_icons[i].y + 72};
        if (PtInRect(&iconRc, pt))
            return (int)i;
    }
    return -1;
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
            case WM_ERASEBKGND:
                return 1;
            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                pThis->DrawWallpaper(hdc);
                pThis->DrawIcons(hdc);
                EndPaint(hwnd, &ps);
                return 0;
            }
            case WM_CONTEXTMENU: {
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                int hit = pThis->HitTestIcon(pt);
                if (hit >= 0) {
                    pThis->ShowItemMenu(hwnd, pt, hit);
                } else {
                    ShowDesktopContextMenu(hwnd, pt);
                }
                return 0;
            }
            case WM_LBUTTONDBLCLK: {
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                int hit = pThis->HitTestIcon(pt);
                if (hit >= 0) {
                    pThis->LaunchItem(hit);
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
