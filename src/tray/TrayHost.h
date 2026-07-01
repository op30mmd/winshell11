#pragma once
#include <map>
#include <string>
#include <vector>
#include <windows.h>

namespace shell::tray {

struct TrayIconData {
    HWND hWnd;
    UINT uID;
    UINT uCallbackMessage;
    HICON hIcon;
    std::wstring szTip;
    UINT uVersion;
};

class TrayHost {
public:
    struct IconKey {
        HWND hWnd;
        UINT uID;
        bool operator<(const IconKey& other) const {
            if (hWnd != other.hWnd)
                return hWnd < other.hWnd;
            return uID < other.uID;
        }
    };

    void Shutdown();
    void Render(HDC hdc, int x, int y, int height);
    int HitTest(POINT pt) const;
    void HandleClick(int index, bool rightButton);

    bool AddIcon(TrayIconData& data);
    bool ModifyIcon(const TrayIconData& data);
    bool DeleteIcon(HWND hWnd, UINT uID);
    int IconCount() const { return (int)m_order.size(); }
    int GetDesiredWidth() const { return IconCount() * (m_iconSize + m_padding); }

    // Called from a parent's WM_COPYDATA handler
    LRESULT HandleCopyData(PCOPYDATASTRUCT pcds);

private:
    std::map<IconKey, TrayIconData> m_icons;
    std::vector<IconKey> m_order;
    int m_iconSize = 16;
    int m_padding = 2;
};

} // namespace shell::tray
