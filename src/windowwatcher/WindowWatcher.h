#pragma once
#include <functional>
#include <string>
#include <vector>
#include <windows.h>

namespace shell::watcher {

struct WindowInfo {
    HWND hwnd;
    std::wstring title;
    HICON icon;
};

class WindowWatcher {
public:
    using Callback = std::function<void()>;

    bool Start(Callback onUpdate, Callback onForegroundChange = nullptr);
    void Stop();

    const std::vector<WindowInfo>& GetWindows() const { return m_windows; }

private:
    static void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild,
                                      DWORD dwEventThread, DWORD dwmsEventTime);

    void RefreshWindowList();
    bool IsTaskbarEligible(HWND hwnd);
    static bool WindowsChanged(const std::vector<WindowInfo>& a, const std::vector<WindowInfo>& b);

    HWINEVENTHOOK m_hook = nullptr;
    HWINEVENTHOOK m_foregroundHook = nullptr;
    std::vector<WindowInfo> m_windows;
    Callback m_onUpdate;
    Callback m_onForegroundChange;
    static WindowWatcher* s_instance;
};

} // namespace shell::watcher
