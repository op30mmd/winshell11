#include "WindowWatcher.h"
#include "common/Logger.h"

namespace shell::watcher {

WindowWatcher* WindowWatcher::s_instance = nullptr;

bool WindowWatcher::Start(Callback onUpdate) {
    s_instance = this;
    m_onUpdate = onUpdate;

    m_hook = SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_NAMECHANGE, nullptr, WinEventProc, 0, 0,
                             WINEVENT_OUTOFCONTEXT);

    RefreshWindowList();
    return m_hook != nullptr;
}

void WindowWatcher::Stop() {
    if (m_hook) {
        UnhookWinEvent(m_hook);
        m_hook = nullptr;
    }
}

void CALLBACK WindowWatcher::WinEventProc(HWINEVENTHOOK /*hWinEventHook*/, DWORD /*event*/, HWND /*hwnd*/,
                                          LONG idObject, LONG idChild, DWORD /*dwEventThread*/,
                                          DWORD /*dwmsEventTime*/) {

    if (s_instance && idObject == OBJID_WINDOW && idChild == CHILDID_SELF) {
        s_instance->RefreshWindowList();
        if (s_instance->m_onUpdate) {
            s_instance->m_onUpdate();
        }
    }
}

void WindowWatcher::RefreshWindowList() {
    m_windows.clear();
    EnumWindows(
        [](HWND hwnd, LPARAM lParam) -> BOOL {
            auto pThis = (WindowWatcher*)lParam;
            if (pThis->IsTaskbarEligible(hwnd)) {
                WindowInfo info;
                info.hwnd = hwnd;
                wchar_t title[256];
                GetWindowTextW(hwnd, title, 256);
                info.title = title;
                pThis->m_windows.push_back(info);
            }
            return TRUE;
        },
        (LPARAM)this);
}

bool WindowWatcher::IsTaskbarEligible(HWND hwnd) {
    if (!IsWindowVisible(hwnd))
        return false;
    HWND owner = GetWindow(hwnd, GW_OWNER);
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    if ((exStyle & WS_EX_TOOLWINDOW) && !(exStyle & WS_EX_APPWINDOW))
        return false;
    if (owner != nullptr && !(exStyle & WS_EX_APPWINDOW))
        return false;
    return true;
}

} // namespace shell::watcher
