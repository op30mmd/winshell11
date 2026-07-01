#include "TrayHost.h"
#include "common/Logger.h"
#include <shellapi.h>

namespace shell::tray {

static TrayHost::IconKey MakeKey(HWND hWnd, UINT uID) {
    return {hWnd, uID};
}

void TrayHost::Shutdown() {
    for (auto& [key, data] : m_icons) {
        if (data.hIcon)
            DestroyIcon(data.hIcon);
    }
    m_icons.clear();
    m_order.clear();
}

void TrayHost::Render(HDC hdc, int x, int y, int height) {
    if (m_order.empty())
        return;

    int iconY = y + (height - m_iconSize) / 2;

    for (size_t i = 0; i < m_order.size(); i++) {
        auto it = m_icons.find(m_order[i]);
        if (it == m_icons.end())
            continue;

        int iconX = x + (int)i * (m_iconSize + m_padding);
        DrawIconEx(hdc, iconX, iconY, it->second.hIcon, m_iconSize, m_iconSize, 0, nullptr, DI_NORMAL);
    }
}

int TrayHost::HitTest(POINT pt) const {
    for (size_t i = 0; i < m_order.size(); i++) {
        int iconX = (int)i * (m_iconSize + m_padding);
        RECT iconRc = {iconX, 0, iconX + m_iconSize + m_padding, m_iconSize + m_padding};
        if (PtInRect(&iconRc, pt)) {
            return (int)i;
        }
    }
    return -1;
}

void TrayHost::HandleClick(int index, bool rightButton) {
    if (index < 0 || index >= (int)m_order.size())
        return;

    auto it = m_icons.find(m_order[index]);
    if (it == m_icons.end())
        return;

    const auto& data = it->second;
    if (data.hWnd && data.uCallbackMessage) {
        PostMessageW(data.hWnd, data.uCallbackMessage, data.uID,
                     rightButton ? WM_RBUTTONDOWN : WM_LBUTTONDOWN);
    }
}

bool TrayHost::AddIcon(TrayIconData& data) {
    if (!data.hIcon)
        data.hIcon = LoadIconW(nullptr, MAKEINTRESOURCEW(32512));

    IconKey key = MakeKey(data.hWnd, data.uID);
    auto [it, inserted] = m_icons.insert({key, data});
    if (inserted)
        m_order.push_back(key);
    return inserted;
}

bool TrayHost::ModifyIcon(const TrayIconData& data) {
    IconKey key = MakeKey(data.hWnd, data.uID);
    auto it = m_icons.find(key);
    if (it == m_icons.end())
        return false;

    if (data.hIcon) {
        DestroyIcon(it->second.hIcon);
        it->second.hIcon = data.hIcon;
    }
    if (!data.szTip.empty())
        it->second.szTip = data.szTip;

    return true;
}

bool TrayHost::DeleteIcon(HWND hWnd, UINT uID) {
    IconKey key = MakeKey(hWnd, uID);
    auto it = m_icons.find(key);
    if (it == m_icons.end())
        return false;

    DestroyIcon(it->second.hIcon);
    m_icons.erase(it);

    for (auto oit = m_order.begin(); oit != m_order.end(); ++oit) {
        if (oit->hWnd == hWnd && oit->uID == uID) {
            m_order.erase(oit);
            break;
        }
    }

    return true;
}

LRESULT TrayHost::HandleCopyData(PCOPYDATASTRUCT pcds) {
    if (!pcds || !pcds->lpData)
        return FALSE;

    DWORD dwMessage = (DWORD)pcds->dwData;

    NOTIFYICONDATAW nid = {};
    ZeroMemory(&nid, sizeof(nid));
    size_t copySize = min((size_t)pcds->cbData, sizeof(nid));
    if (copySize < sizeof(NOTIFYICONDATAW) - sizeof(nid.szInfo) - sizeof(nid.szInfoTitle))
        return FALSE;

    CopyMemory(&nid, pcds->lpData, copySize);

    switch (dwMessage) {
        case NIM_ADD: {
            TrayIconData data;
            data.hWnd = nid.hWnd;
            data.uID = nid.uID;
            data.uCallbackMessage = nid.uCallbackMessage;
            data.uVersion = NOTIFYICON_VERSION;
            data.szTip = nid.szTip[0] ? nid.szTip : L"";
            data.hIcon = nid.hIcon ? CopyIcon(nid.hIcon) : LoadIconW(nullptr, MAKEINTRESOURCEW(32512));
            AddIcon(data);
            return TRUE;
        }
        case NIM_MODIFY: {
            TrayIconData data;
            data.hWnd = nid.hWnd;
            data.uID = nid.uID;
            data.uCallbackMessage = nid.uCallbackMessage;
            data.szTip = nid.szTip;
            data.hIcon = nid.hIcon ? CopyIcon(nid.hIcon) : nullptr;
            ModifyIcon(data);
            return TRUE;
        }
        case NIM_DELETE: {
            DeleteIcon(nid.hWnd, nid.uID);
            return TRUE;
        }
        case NIM_SETVERSION: {
            IconKey key = MakeKey(nid.hWnd, nid.uID);
            auto it = m_icons.find(key);
            if (it != m_icons.end())
                it->second.uVersion = (UINT)nid.uCallbackMessage;
            return TRUE;
        }
    }

    return FALSE;
}

} // namespace shell::tray
