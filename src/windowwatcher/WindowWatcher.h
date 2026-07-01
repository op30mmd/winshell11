#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <functional>

namespace shell::watcher {

struct WindowInfo {
    HWND hwnd;
    std::wstring title;
    HICON icon;
};

class WindowWatcher {
public:
    using Callback = std::function<void()>;

    bool Start(Callback onUpdate);
    void Stop();

    const std::vector<WindowInfo>& GetWindows() const { return m_windows; }

private:
    static void CALLBACK WinEventProc(
        HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd,
        LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);

    void RefreshWindowList();
    bool IsTaskbarEligible(HWND hwnd);

    HWINEVENTHOOK m_hook = nullptr;
    std::vector<WindowInfo> m_windows;
    Callback m_onUpdate;
    static WindowWatcher* s_instance;
};

} // namespace shell::watcher
