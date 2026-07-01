#include "WindowWatcher.h"
#include "common/Logger.h"
#include <algorithm>

namespace shell::watcher {

WindowWatcher* WindowWatcher::s_instance = nullptr;

bool WindowWatcher::Start(Callback onUpdate, Callback onForegroundChange) {
    s_instance = this;
    m_onUpdate = onUpdate;
    m_onForegroundChange = onForegroundChange;

    // Hook window creation/destruction only (not location/state changes which fire constantly)
    m_hook = SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_DESTROY, nullptr, WinEventProc, 0, 0,
                             WINEVENT_OUTOFCONTEXT);

    // Hook foreground changes separately to update taskbar active state
    m_foregroundHook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr, WinEventProc, 0, 0,
                                       WINEVENT_OUTOFCONTEXT);

    RefreshWindowList();

    // Fire initial update so taskbar shows windows immediately
    if (m_onUpdate)
        m_onUpdate();
    if (m_onForegroundChange)
        m_onForegroundChange();

    return m_hook != nullptr;
}

void WindowWatcher::Stop() {
    if (m_hook) {
        UnhookWinEvent(m_hook);
        m_hook = nullptr;
    }
    if (m_foregroundHook) {
        UnhookWinEvent(m_foregroundHook);
        m_foregroundHook = nullptr;
    }
}

void CALLBACK WindowWatcher::WinEventProc(HWINEVENTHOOK /*hWinEventHook*/, DWORD event, HWND /*hwnd*/,
                                          LONG idObject, LONG idChild, DWORD /*dwEventThread*/,
                                          DWORD /*dwmsEventTime*/) {

    if (!s_instance)
        return;

    if (event == EVENT_SYSTEM_FOREGROUND) {
        if (s_instance->m_onForegroundChange)
            s_instance->m_onForegroundChange();
        return;
    }

    if (idObject == OBJID_WINDOW && idChild == CHILDID_SELF) {
        auto oldList = s_instance->m_windows;
        s_instance->RefreshWindowList();

        if (s_instance->m_onUpdate && WindowsChanged(oldList, s_instance->m_windows)) {
            s_instance->m_onUpdate();
        }
    }
}

bool WindowWatcher::WindowsChanged(const std::vector<WindowInfo>& a, const std::vector<WindowInfo>& b) {
    if (a.size() != b.size())
        return true;
    for (size_t i = 0; i < a.size(); i++) {
        if (a[i].hwnd != b[i].hwnd)
            return true;
    }
    return false;
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
                title[0] = L'\0';
                GetWindowTextW(hwnd, title, 256);
                info.title = title;
                info.icon = nullptr;
                pThis->m_windows.push_back(info);
            }
            return TRUE;
        },
        (LPARAM)this);

    // Sort by HWND for stable order (EnumWindows uses Z-order which changes on foreground)
    std::sort(m_windows.begin(), m_windows.end(),
              [](const WindowInfo& a, const WindowInfo& b) { return a.hwnd < b.hwnd; });
}

bool WindowWatcher::IsTaskbarEligible(HWND hwnd) {
    if (!IsWindowVisible(hwnd))
        return false;
    HWND owner = GetWindow(hwnd, GW_OWNER);
    LONG exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
    if ((exStyle & WS_EX_TOOLWINDOW) && !(exStyle & WS_EX_APPWINDOW))
        return false;
    if (owner != nullptr && !(exStyle & WS_EX_APPWINDOW))
        return false;
    return true;
}

} // namespace shell::watcher
