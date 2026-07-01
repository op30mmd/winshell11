#include "TaskbarWindow.h"
#include "tray/TrayHost.h"
#include "common/Logger.h"
#include <shellapi.h>
#include <windowsx.h>

namespace shell::taskbar {

int TaskbarWindow::GetTrayWidth() const {
    return m_trayHost ? m_trayHost->GetDesiredWidth() : 0;
}

void TaskbarWindow::RepositionTrayArea() {
    // Tray area is positioned automatically during WM_PAINT.
    // Just trigger a repaint.
    if (m_hwnd)
        InvalidateRect(m_hwnd, nullptr, TRUE);
}

void TaskbarWindow::DrawTrayIcons(HDC hdc, int width, int height) {
    if (!m_trayHost || m_trayHost->IconCount() == 0)
        return;

    int trayWidth = GetTrayWidth();
    int trayX = width - m_clockWidth - 6 - trayWidth;
    m_trayHost->Render(hdc, trayX, 0, height);
}

int TaskbarWindow::GetActiveIndex() const {
    HWND fg = GetForegroundWindow();
    for (size_t i = 0; i < m_windows.size(); i++) {
        if (m_windows[i].hwnd == fg)
            return (int)i;
    }
    return -1;
}

void TaskbarWindow::CheckActiveWindow() {
    int activeIdx = GetActiveIndex();
    if (activeIdx == m_nextActiveIdx)
        return;

    m_prevActiveIdx = m_nextActiveIdx;
    m_nextActiveIdx = activeIdx;
    m_animStartTick = GetTickCount64();
    if (m_hwnd)
        SetTimer(m_hwnd, 2, 16, nullptr);
}

void TaskbarWindow::EnsureBuffer(int w, int h) {
    if (m_cacheDC && m_cacheW >= w && m_cacheH >= h)
        return;
    DestroyBuffer();
    HDC screenDC = GetDC(nullptr);
    m_cacheDC = CreateCompatibleDC(screenDC);
    m_cacheBmp = CreateCompatibleBitmap(screenDC, w, h);
    m_cacheOldBmp = (HBITMAP)SelectObject(m_cacheDC, m_cacheBmp);
    m_cacheW = w;
    m_cacheH = h;
    ReleaseDC(nullptr, screenDC);
}

void TaskbarWindow::DestroyBuffer() {
    if (m_cacheDC) {
        SelectObject(m_cacheDC, m_cacheOldBmp);
        DeleteObject(m_cacheBmp);
        DeleteDC(m_cacheDC);
        m_cacheDC = nullptr;
        m_cacheBmp = nullptr;
        m_cacheOldBmp = nullptr;
        m_cacheW = 0;
        m_cacheH = 0;
    }
}

void TaskbarWindow::SetWindows(const std::vector<watcher::WindowInfo>& windows) {
    m_windows = windows;
    if (m_hwnd) {
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }
}

std::wstring TaskbarWindow::GetTimeString() const {
    wchar_t buf[64];
    GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, nullptr, nullptr, buf, 64);
    wchar_t dateBuf[64];
    GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, nullptr, nullptr, dateBuf, 64);
    return std::wstring(buf) + L"\n" + dateBuf;
}

bool TaskbarWindow::Create(HINSTANCE hInstance, const common::TaskbarConfig& config) {
    m_config = config;
    const wchar_t CLASS_NAME[] = L"ShellTaskbarWindowClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassW(&wc);

    int height = 40;
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    m_hwnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, CLASS_NAME, L"Taskbar", WS_POPUP | WS_VISIBLE, 0,
                             screenHeight - height, screenWidth, height, nullptr, nullptr, hInstance, this);

    if (m_hwnd) {
        RegisterAppBar(true);
        SetTimer(m_hwnd, 1, 1000, nullptr);
    }

    return m_hwnd != nullptr;
}

void TaskbarWindow::RegisterAppBar(bool registerIt) {
    APPBARDATA abd = {sizeof(abd), m_hwnd};
    if (registerIt) {
        m_appbarMsg = RegisterWindowMessageW(L"AppBarMessage");
        abd.uCallbackMessage = m_appbarMsg;
        SHAppBarMessage(ABM_NEW, &abd);

        // Sequence: QUERYPOS → adjust → SETPOS
        abd.uEdge = ABE_BOTTOM;
        abd.rc.left = 0;
        abd.rc.top = GetSystemMetrics(SM_CYSCREEN) - 40;
        abd.rc.right = GetSystemMetrics(SM_CXSCREEN);
        abd.rc.bottom = GetSystemMetrics(SM_CYSCREEN);
        SHAppBarMessage(ABM_QUERYPOS, &abd);
        SetWindowPos(m_hwnd, HWND_TOPMOST, abd.rc.left, abd.rc.top,
                     abd.rc.right - abd.rc.left, abd.rc.bottom - abd.rc.top,
                     SWP_NOACTIVATE);
        SHAppBarMessage(ABM_SETPOS, &abd);

        // Directly set the work area above the taskbar
        RECT workRc = {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) - 40};
        SystemParametersInfo(SPI_SETWORKAREA, 1, &workRc, SPIF_SENDCHANGE);
    } else {
        SHAppBarMessage(ABM_REMOVE, &abd);

        // Restore full-screen work area
        RECT fullRc = {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
        SystemParametersInfo(SPI_SETWORKAREA, 1, &fullRc, SPIF_SENDCHANGE);
    }
}

void TaskbarWindow::DrawStartButton(HDC hdc, int height) {
    RECT startRc = {0, 0, m_startBtnWidth, height};

    HBRUSH hBrush = CreateSolidBrush(RGB(0, 120, 215));
    FillRect(hdc, &startRc, hBrush);
    DeleteObject(hBrush);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    HFONT hFont = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                               CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    DrawTextW(hdc, L"\u25B6", -1, &startRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

void TaskbarWindow::DrawClock(HDC hdc, int width, int height) {
    RECT clockRc = {width - m_clockWidth, 0, width, height};

    HBRUSH hBrush = CreateSolidBrush(RGB(30, 30, 35));
    FillRect(hdc, &clockRc, hBrush);
    DeleteObject(hBrush);

    RECT sepRc = {width - m_clockWidth - 1, 4, width - m_clockWidth, height - 4};
    HBRUSH hSep = CreateSolidBrush(RGB(60, 60, 65));
    FillRect(hdc, &sepRc, hSep);
    DeleteObject(hSep);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(220, 220, 220));
    HFONT hFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                               CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    RECT timeRc = clockRc;
    timeRc.top += 3;
    timeRc.bottom -= 2;
    std::wstring timeStr = GetTimeString();
    size_t newlinePos = timeStr.find(L'\n');
    std::wstring timeLine = timeStr.substr(0, newlinePos);
    std::wstring dateLine = timeStr.substr(newlinePos + 1);

    DrawTextW(hdc, timeLine.c_str(), -1, &timeRc, DT_CENTER | DT_TOP | DT_SINGLELINE);
    timeRc.top += 17;
    DrawTextW(hdc, dateLine.c_str(), -1, &timeRc, DT_CENTER | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

void TaskbarWindow::DrawButtons(HDC hdc, int startX, int endX, int height) {
    ULONGLONG now = GetTickCount64();
    bool animating = (m_animStartTick > 0 && now - m_animStartTick < ANIM_DURATION_MS);
    float t = 0.0f;
    if (animating)
        t = min(1.0f, (float)(now - m_animStartTick) / ANIM_DURATION_MS);

    int x = startX;
    for (size_t i = 0; i < m_windows.size(); i++) {
        int w = m_buttonWidth;
        if (x + w > endX)
            w = endX - x;
        if (w < 40)
            break;

        RECT btnRc = {(int)x, 4, (int)(x + w), height - 4};

        int accentH = 0;
        bool buttonActive = false;
        bool hovered = ((int)i == m_hoveredIdx);

        if (animating) {
            if ((int)i == m_prevActiveIdx) {
                accentH = (int)(3.0f * (1.0f - t) + 0.5f);
                buttonActive = (accentH > 1);
            } else if ((int)i == m_nextActiveIdx) {
                accentH = (int)(3.0f * t + 0.5f);
                buttonActive = (accentH > 0);
            }
        } else {
            buttonActive = ((int)i == GetActiveIndex());
            accentH = buttonActive ? 3 : 0;
        }

        // Background color fades: active > hovered > normal
        int r, g, b;
        if (buttonActive || (animating && (int)i == m_prevActiveIdx)) {
            r = hovered ? 60 : 55; g = hovered ? 60 : 55; b = hovered ? 70 : 65;
        } else if (hovered) {
            r = 50; g = 50; b = 56;
        } else {
            r = 40; g = 40; b = 48;
        }
        HBRUSH hBrush = CreateSolidBrush(RGB(r, g, b));
        FillRect(hdc, &btnRc, hBrush);
        DeleteObject(hBrush);

        if (accentH > 0) {
            RECT accentRc = {(int)x, 0, (int)(x + w), accentH};
            HBRUSH hAccent = CreateSolidBrush(RGB(0, 120, 215));
            FillRect(hdc, &accentRc, hAccent);
            DeleteObject(hAccent);
        }

        RECT textRc = btnRc;
        textRc.left += 8;
        textRc.right -= 4;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(220, 220, 220));
        HFONT hFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                   CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        DrawTextW(hdc, m_windows[i].title.c_str(), -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);

        x += w + 2;
    }
}

int TaskbarWindow::HitTest(POINT pt) const {
    RECT rc;
    GetClientRect(m_hwnd, &rc);

    if (pt.x < m_startBtnWidth)
        return -2;

    int trayWidth = GetTrayWidth();
    int btnStart = m_startBtnWidth + 4;
    int btnEnd = rc.right - m_clockWidth - 6 - trayWidth;
    int x = btnStart;

    for (size_t i = 0; i < m_windows.size(); i++) {
        int w = m_buttonWidth;
        if (x + w > btnEnd)
            w = btnEnd - x;
        if (w < 40)
            break;

        if (pt.x >= x && pt.x <= x + w) {
            return (int)i;
        }
        x += w + 2;
    }

    // Check tray area
    int trayStart = rc.right - m_clockWidth - 6 - trayWidth;
    if (pt.x >= trayStart && pt.x < rc.right - m_clockWidth - 6)
        return -3;
    return -1;
}

void TaskbarWindow::ShowWindowMenu(HWND hwnd, POINT pt, int windowIndex) {
    if (windowIndex < 0 || windowIndex >= (int)m_windows.size())
        return;

    HWND hTarget = m_windows[windowIndex].hwnd;
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu)
        return;

    AppendMenuW(hMenu, MF_STRING, 1, L"Restore");
    AppendMenuW(hMenu, MF_STRING, 2, L"Minimize");
    AppendMenuW(hMenu, MF_STRING, 3, L"Maximize");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, 4, L"Close");

    UINT cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
    DestroyMenu(hMenu);

    switch (cmd) {
        case 1:
            ShowWindow(hTarget, SW_RESTORE);
            SetForegroundWindow(hTarget);
            break;
        case 2:
            ShowWindow(hTarget, SW_MINIMIZE);
            break;
        case 3:
            ShowWindow(hTarget, SW_MAXIMIZE);
            break;
        case 4:
            PostMessageW(hTarget, WM_CLOSE, 0, 0);
            break;
    }
}

void TaskbarWindow::ActivateWindow(int index) {
    if (index < 0 || index >= (int)m_windows.size())
        return;

    HWND hTarget = m_windows[index].hwnd;
    if (IsIconic(hTarget)) {
        ShowWindow(hTarget, SW_RESTORE);
    }
    SetForegroundWindow(hTarget);
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
            case WM_ERASEBKGND:
                return 1;
            case WM_MOUSEACTIVATE:
                return MA_NOACTIVATE;
            case WM_MOUSEMOVE: {
                TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hwnd, 0};
                TrackMouseEvent(&tme);
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                int hit = pThis->HitTest(pt);
                if (hit != pThis->m_hoveredIdx) {
                    pThis->m_hoveredIdx = hit;
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;
            }
            case WM_MOUSELEAVE: {
                pThis->m_hoveredIdx = -1;
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            case WM_TIMER:
                if (pThis->m_animStartTick > 0) {
                    ULONGLONG now = GetTickCount64();
                    if (now - pThis->m_animStartTick >= pThis->ANIM_DURATION_MS) {
                        pThis->m_animStartTick = 0;
                        KillTimer(hwnd, 2);
                    }
                    InvalidateRect(hwnd, nullptr, FALSE);
                } else if (wParam == 1) {
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;
            case WM_PAINT: {
                pThis->CheckActiveWindow();
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                RECT rc;
                GetClientRect(hwnd, &rc);
                int w = rc.right - rc.left;
                int h = rc.bottom - rc.top;

                pThis->EnsureBuffer(w, h);

                HBRUSH hBg = CreateSolidBrush(RGB(25, 25, 30));
                FillRect(pThis->m_cacheDC, &rc, hBg);
                DeleteObject(hBg);

                int trayWidth = pThis->GetTrayWidth();
                int btnEnd = w - pThis->m_clockWidth - 6 - trayWidth;
                pThis->DrawStartButton(pThis->m_cacheDC, h);
                pThis->DrawButtons(pThis->m_cacheDC, pThis->m_startBtnWidth + 4, btnEnd, h);
                pThis->DrawTrayIcons(pThis->m_cacheDC, w, h);
                pThis->DrawClock(pThis->m_cacheDC, w, h);

                BitBlt(hdc, 0, 0, w, h, pThis->m_cacheDC, 0, 0, SRCCOPY);

                EndPaint(hwnd, &ps);
                return 0;
            }
            case WM_LBUTTONDOWN: {
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                int hit = pThis->HitTest(pt);
                if (hit == -2) {
                    if (pThis->m_onStartClick) {
                        pThis->m_onStartClick();
                    }
                } else if (hit == -3) {
                    if (pThis->m_trayHost) {
                        RECT rc;
                        GetClientRect(hwnd, &rc);
                        int trayWidth = pThis->GetTrayWidth();
                        int trayX = rc.right - pThis->m_clockWidth - 6 - trayWidth;
                        POINT trayPt = {pt.x - trayX, pt.y};
                        int iconHit = pThis->m_trayHost->HitTest(trayPt);
                        pThis->m_trayHost->HandleClick(iconHit, false);
                    }
                } else if (hit >= 0) {
                    pThis->ActivateWindow(hit);
                }
                return 0;
            }
            case WM_RBUTTONDOWN: {
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                int hit = pThis->HitTest(pt);
                if (hit >= 0) {
                    ClientToScreen(hwnd, &pt);
                    pThis->ShowWindowMenu(hwnd, pt, hit);
                } else if (hit == -3) {
                    if (pThis->m_trayHost) {
                        RECT rc;
                        GetClientRect(hwnd, &rc);
                        int trayWidth = pThis->GetTrayWidth();
                        int trayX = rc.right - pThis->m_clockWidth - 6 - trayWidth;
                        POINT trayPt = {pt.x - trayX, pt.y};
                        int iconHit = pThis->m_trayHost->HitTest(trayPt);
                        pThis->m_trayHost->HandleClick(iconHit, true);
                    }
                }
                return 0;
            }
            case WM_WINDOWPOSCHANGING: {
                WINDOWPOS* wp = (WINDOWPOS*)lParam;
                if (!(wp->flags & SWP_NOZORDER))
                    wp->hwndInsertAfter = HWND_TOPMOST;
                return DefWindowProcW(hwnd, uMsg, wParam, lParam);
            }
            case WM_SIZE:
                pThis->DestroyBuffer();
                InvalidateRect(hwnd, nullptr, TRUE);
                return 0;
            case WM_COPYDATA: {
                if (pThis->m_trayHost) {
                    PCOPYDATASTRUCT pcds = (PCOPYDATASTRUCT)lParam;
                    LRESULT result = pThis->m_trayHost->HandleCopyData(pcds);
                    if (result)
                        InvalidateRect(hwnd, nullptr, TRUE);
                    return result;
                }
                return FALSE;
            }
            case WM_SYSCOMMAND:
                if (wParam == SC_MINIMIZE || wParam == SC_CLOSE || wParam == SC_SCREENSAVE)
                    return 0;
                return DefWindowProcW(hwnd, uMsg, wParam, lParam);
            default:
                if (uMsg == pThis->m_appbarMsg) {
                    if (wParam == ABN_FULLSCREENAPP) {
                        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    } else if (wParam == ABN_POSCHANGED) {
                        // System is renegotiating appbar positions
                        APPBARDATA abd = {sizeof(abd), hwnd};
                        abd.uEdge = ABE_BOTTOM;
                        abd.rc.left = 0;
                        abd.rc.top = GetSystemMetrics(SM_CYSCREEN) - 40;
                        abd.rc.right = GetSystemMetrics(SM_CXSCREEN);
                        abd.rc.bottom = GetSystemMetrics(SM_CYSCREEN);
                        SHAppBarMessage(ABM_QUERYPOS, &abd);
                        SetWindowPos(hwnd, HWND_TOPMOST, abd.rc.left, abd.rc.top,
                                     abd.rc.right - abd.rc.left, abd.rc.bottom - abd.rc.top,
                                     SWP_NOACTIVATE);
                        SHAppBarMessage(ABM_SETPOS, &abd);
                    }
                    return 0;
                }
                break;
            case WM_CLOSE:
                return 0;
            case WM_DESTROY:
                KillTimer(hwnd, 1);
                KillTimer(hwnd, 2);
                pThis->DestroyBuffer();
                pThis->RegisterAppBar(false);
                return 0;
        }
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

} // namespace shell::taskbar
