#pragma once
#include <functional>
#include <map>
#include <windows.h>

namespace shell::host {

class HotkeyManager {
public:
    using HotkeyCallback = std::function<void()>;

    bool Initialize(HWND hwnd);
    void Register(int id, UINT modifiers, UINT vk, HotkeyCallback callback);
    void HandleMessage(int id);

private:
    HWND m_hwnd = nullptr;
    std::map<int, HotkeyCallback> m_callbacks;
};

} // namespace shell::host
