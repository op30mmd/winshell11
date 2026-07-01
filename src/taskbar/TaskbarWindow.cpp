#include "TaskbarWindow.h"
#include "common/Logger.h"
#include <shellapi.h>
#include <windowsx.h>

namespace shell::taskbar {

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
    int x = startX;
    for (size_t i = 0; i < m_windows.size(); i++) {
        int w = m_buttonWidth;
        if (x + w > endX)
            w = endX - x;
        if (w < 40)
            break;

        RECT btnRc = {x, 4, x + w, height - 4};
        HWND fg = GetForegroundWindow();
        bool active = (m_windows[i].hwnd == fg);

        HBRUSH hBrush = CreateSolidBrush(active ? RGB(55, 55, 65) : RGB(40, 40, 48));
        FillRect(hdc, &btnRc, hBrush);
        DeleteObject(hBrush);

        if (active) {
            RECT accentRc = {x, 0, x + w, 3};
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

    int btnStart = m_startBtnWidth + 4;
    int btnEnd = rc.right - m_clockWidth - 6;
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
    return -1;
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
            case WM_TIMER:
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                RECT rc;
                GetClientRect(hwnd, &rc);
                int w = rc.right - rc.left;
                int h = rc.bottom - rc.top;

                HBRUSH hBg = CreateSolidBrush(RGB(25, 25, 30));
                FillRect(hdc, &rc, hBg);
                DeleteObject(hBg);

                pThis->DrawStartButton(hdc, h);
                pThis->DrawButtons(hdc, pThis->m_startBtnWidth + 4, w - pThis->m_clockWidth - 6, h);
                pThis->DrawClock(hdc, w, h);
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
                } else if (hit >= 0) {
                    pThis->ActivateWindow(hit);
                }
                return 0;
            }
            case WM_DESTROY:
                KillTimer(hwnd, 1);
                pThis->RegisterAppBar(false);
                return 0;
        }
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

} // namespace shell::taskbar
