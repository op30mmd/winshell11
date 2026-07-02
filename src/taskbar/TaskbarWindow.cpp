#include "TaskbarWindow.h"
#include "flyout/FlyoutWindow.h"
#include "tray/TrayHost.h"
#include "common/Logger.h"
#include <shellapi.h>
#include <windowsx.h>

namespace shell::taskbar {

int TaskbarWindow::GetTrayWidth() const {
    return m_trayHost ? m_trayHost->GetDesiredWidth() : 0;
}

void TaskbarWindow::RepositionTrayArea() {
    if (m_hwnd)
        InvalidateRect(m_hwnd, nullptr, TRUE);
}

void TaskbarWindow::CheckActiveWindow() {
    auto* panel = static_cast<TaskbarPanel*>(m_ui.GetRoot());
    auto* btns = panel ? panel->GetTaskButtons() : nullptr;
    if (!btns)
        return;
    HWND fg = GetForegroundWindow();
    int activeIdx = -1;
    for (size_t i = 0; i < m_windows.size(); i++) {
        if (m_windows[i].hwnd == fg) {
            activeIdx = (int)i;
            break;
        }
    }
    btns->SetActiveWindow(activeIdx);
}

void TaskbarWindow::SetWindows(const std::vector<watcher::WindowInfo>& windows) {
    m_windows = windows;
    auto* panel = static_cast<TaskbarPanel*>(m_ui.GetRoot());
    if (auto* btns = panel ? panel->GetTaskButtons() : nullptr)
        btns->SetWindows(windows);
    if (m_hwnd)
        InvalidateRect(m_hwnd, nullptr, TRUE);
}

void TaskbarWindow::BuildWidgetTree() {
    auto panel = std::make_unique<TaskbarPanel>();

    // Start button
    auto startBtn = std::make_unique<StartButtonWidget>();
    startBtn->SetOnClick([this]() {
        if (m_onStartClick)
            m_onStartClick();
    });
    panel->SetStartButton(std::move(startBtn));

    // Task buttons
    auto taskBtns = std::make_unique<TaskButtonsContainer>();
    taskBtns->SetWindows(m_windows);
    taskBtns->SetOnWindowClick([this](int idx) {
        ActivateWindow(idx);
    });
    panel->SetTaskButtons(std::move(taskBtns));

    // Tray area
    auto tray = std::make_unique<TrayWidget>();
    tray->SetTrayHost(m_trayHost);
    auto* trayRaw = tray.get();
    tray->SetOnClick([this, trayRaw](POINT pt) {
        if (m_trayHost) {
            const RECT& tb = trayRaw->GetBounds();
            POINT localPt = {pt.x - tb.left, pt.y};
            int iconHit = m_trayHost->HitTest(localPt);
            m_trayHost->HandleClick(iconHit, false);
        }
    });
    panel->SetTray(std::move(tray));

    // Clock
    auto clock = std::make_unique<ClockWidget>();
    clock->SetOnClick([this]() {
        ShowFlyout();
    });
    panel->SetClock(std::move(clock));

    m_ui.SetRoot(std::move(panel));
}

void TaskbarWindow::SetTrayHost(tray::TrayHost* host) {
    m_trayHost = host;
    auto* panel = static_cast<TaskbarPanel*>(m_ui.GetRoot());
    if (auto* tw = panel ? panel->GetTray() : nullptr)
        tw->SetTrayHost(host);
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
        m_ui.Attach(m_hwnd);
        m_ui.GetTheme().background = RGB(25, 25, 30);
        BuildWidgetTree();
    }

    return m_hwnd != nullptr;
}

void TaskbarWindow::RegisterAppBar(bool registerIt) {
    APPBARDATA abd = {sizeof(abd), m_hwnd};
    if (registerIt) {
        m_appbarMsg = RegisterWindowMessageW(L"AppBarMessage");
        abd.uCallbackMessage = m_appbarMsg;
        GetWindowRect(m_hwnd, &abd.rc);
        SHAppBarMessage(ABM_NEW, &abd);

        abd.uEdge = ABE_BOTTOM;
        int sh = GetSystemMetrics(SM_CYSCREEN);
        int sw = GetSystemMetrics(SM_CXSCREEN);
        abd.rc.left = 0;
        abd.rc.top = sh - 40;
        abd.rc.right = sw;
        abd.rc.bottom = sh;
        SHAppBarMessage(ABM_QUERYPOS, &abd);
        // ABM_QUERYPOS after ABM_NEW may return zero height (the work area
        // already clips reserved space), so enforce our desired height.
        abd.rc.bottom = abd.rc.top + 40;
        SetWindowPos(m_hwnd, HWND_TOPMOST, abd.rc.left, abd.rc.top,
                     abd.rc.right - abd.rc.left, abd.rc.bottom - abd.rc.top,
                     SWP_NOACTIVATE);
        SHAppBarMessage(ABM_SETPOS, &abd);
    } else {
        SHAppBarMessage(ABM_REMOVE, &abd);
    }
}

void TaskbarWindow::ShowFlyout() {
    if (!m_flyout)
        return;
    if (IsWindowVisible(m_flyout->GetHWND())) {
        m_flyout->Dismiss();
        return;
    }
    RECT rc;
    GetWindowRect(m_hwnd, &rc);
    m_flyout->Show(m_hwnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
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

// ── Accessors into the widget tree (inline helpers) ────────────────

static TaskbarPanel* GetPanel(ui::UiHost& ui) {
    return static_cast<TaskbarPanel*>(ui.GetRoot());
}

#define PANEL (GetPanel(pThis->m_ui))

// ═══════════════════════════════════════════════════════════════════════
//  Window procedure
// ═══════════════════════════════════════════════════════════════════════

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
        // Pre-process: dismiss flyout on left click (clock will re-show)
        if (uMsg == WM_LBUTTONDOWN) {
            if (pThis->m_flyout) {
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                // Don't dismiss if clock was clicked (clock toggles flyout)
                if (PANEL && PANEL->GetClock()) {
                    RECT clockRc = PANEL->GetClock()->GetBounds();
                    if (!PtInRect(&clockRc, pt))
                        pThis->m_flyout->Dismiss();
                } else {
                    pThis->m_flyout->Dismiss();
                }
            }
        }

        // Route paint, input, and layout through the UI host (once attached).
        LRESULT result = 0;
        if (pThis->m_ui.IsAttached() && pThis->m_ui.HandleMessage(uMsg, wParam, lParam, result))
            return result;

        switch (uMsg) {
            case WM_MOUSEACTIVATE:
                return MA_NOACTIVATE;

            case WM_TIMER:
                if (wParam == 1) {
                    // Clock refresh — repaint every second
                    pThis->CheckActiveWindow();
                    pThis->m_ui.Invalidate();
                }
                return 0;

            case WM_LBUTTONDOWN: {
                // If we get here, nothing consumed it — fallback tray handling
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                if (PANEL && PANEL->GetTray()) {
                    RECT trayRc = PANEL->GetTray()->GetBounds();
                    if (PtInRect(&trayRc, pt) && pThis->m_trayHost) {
                        POINT localPt = {pt.x - trayRc.left, pt.y};
                        int iconHit = pThis->m_trayHost->HitTest(localPt);
                        pThis->m_trayHost->HandleClick(iconHit, false);
                    }
                }
                return 0;
            }

            case WM_RBUTTONDOWN: {
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                // Check tray area
                if (PANEL && PANEL->GetTray()) {
                    RECT trayRc = PANEL->GetTray()->GetBounds();
                    if (PtInRect(&trayRc, pt) && pThis->m_trayHost) {
                        POINT localPt = {pt.x - trayRc.left, pt.y};
                        int iconHit = pThis->m_trayHost->HitTest(localPt);
                        pThis->m_trayHost->HandleClick(iconHit, true);
                        return 0;
                    }
                }
                // Check task buttons for context menu
                if (PANEL && PANEL->GetTaskButtons()) {
                    auto* hit = static_cast<TaskButtonWidget*>(
                        PANEL->GetTaskButtons()->HitTest(pt));
                    if (hit) {
                        // Find the index
                        for (size_t i = 0; i < pThis->m_windows.size(); i++) {
                            if (pThis->m_windows[i].hwnd == hit->GetWindowInfo().hwnd) {
                                ClientToScreen(hwnd, &pt);
                                pThis->ShowWindowMenu(hwnd, pt, (int)i);
                                break;
                            }
                        }
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
                pThis->m_ui.PerformLayout();
                return 0;

            case WM_COPYDATA: {
                if (pThis->m_trayHost) {
                    PCOPYDATASTRUCT pcds = (PCOPYDATASTRUCT)lParam;
                    LRESULT cdResult = pThis->m_trayHost->HandleCopyData(pcds);
                    if (cdResult)
                        pThis->m_ui.Invalidate();
                    return cdResult;
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
                        APPBARDATA abd = {sizeof(abd), hwnd};
                        abd.uEdge = ABE_BOTTOM;
                        abd.rc.left = 0;
                        abd.rc.top = GetSystemMetrics(SM_CYSCREEN) - 40;
                        abd.rc.right = GetSystemMetrics(SM_CXSCREEN);
                        abd.rc.bottom = GetSystemMetrics(SM_CYSCREEN);
                        SHAppBarMessage(ABM_QUERYPOS, &abd);
                        // Enforce minimum height (ABM_QUERYPOS may clip to 0)
                        if (abd.rc.bottom - abd.rc.top < 40)
                            abd.rc.bottom = abd.rc.top + 40;
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
                pThis->RegisterAppBar(false);
                return 0;
        }
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

#undef PANEL

} // namespace shell::taskbar
