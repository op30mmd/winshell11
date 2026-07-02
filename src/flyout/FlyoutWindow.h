#pragma once
#include <string>
#include <windows.h>

namespace shell::flyout {

class FlyoutWindow {
public:
    bool Create(HINSTANCE hInstance);
    HWND GetHWND() const { return m_hwnd; }
    void Show(HWND hwndParent, int taskbarX, int taskbarY, int taskbarW, int taskbarH);
    void Dismiss();

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void OnPaint(HDC hdc);
    int HitTestSection(POINT pt) const;
    void OnClick(int section, HWND hwnd);

    void ToggleWiFi();
    void ToggleBluetooth();
    void SetBrightness(int val);
    void SetVolume(int val);
    std::wstring GetWiFiStatus();
    std::wstring GetBluetoothStatus();
    bool HasBrightnessControl();
    bool HasBluetoothHardware();
    int GetCurrentBrightness();
    int GetCurrentVolume();
    std::wstring GetNetworkInfo();

    HWND m_hwnd = nullptr;
    HWND m_hwndParent = nullptr;

    int m_width = 360;
    int m_height = 480;

    int m_volume = 75;
    int m_brightness = 80;
    bool m_wifiOn = true;
    bool m_btOn = false;
    bool m_hasBrightness = false;
    bool m_hasBluetooth = false;

    bool m_draggingVolume = false;
    bool m_draggingBrightness = false;
};

} // namespace shell::flyout
