#pragma once
#include <windows.h>
#include <vector>
#include <map>
#include <string>

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
    bool Initialize(HWND hTaskbar);
    void Shutdown();

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    struct IconKey {
        HWND hWnd;
        UINT uID;
        bool operator<(const IconKey& other) const {
            if (hWnd != other.hWnd) return hWnd < other.hWnd;
            return uID < other.uID;
        }
    };

    HWND m_hwnd = nullptr;
    std::map<IconKey, TrayIconData> m_icons;
};

} // namespace shell::tray
