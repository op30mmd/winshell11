#include "HotkeyManager.h"

namespace shell::host {

bool HotkeyManager::Initialize(HWND hwnd) {
    m_hwnd = hwnd;
    return true;
}

void HotkeyManager::Register(int id, UINT modifiers, UINT vk, HotkeyCallback callback) {
    if (RegisterHotKey(m_hwnd, id, modifiers, vk)) {
        m_callbacks[id] = callback;
    }
}

void HotkeyManager::HandleMessage(int id) {
    auto it = m_callbacks.find(id);
    if (it != m_callbacks.end()) {
        it->second();
    }
}

} // namespace shell::host
