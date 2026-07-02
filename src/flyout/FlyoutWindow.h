#pragma once
#include "ui/UiHost.h"
#include "ui/Controls.h"
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

    // Builds (or rebuilds) the widget tree from current state.
    void RebuildWidgetTree();

    void ToggleWiFi();
    void ToggleBluetooth();
    void SetBrightness(int val);
    void SetVolume(int val);
    std::wstring GetWiFiStatus();
    std::wstring GetBluetoothStatus();
    bool HasBrightnessControl();
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

    // New UI framework host
    shell::ui::UiHost m_ui;

    // Pointers into the widget tree for live updates (owned by m_ui).
    shell::ui::Label*  m_wifiLabel        = nullptr;
    shell::ui::Label*  m_btLabel          = nullptr;
    shell::ui::Slider* m_volumeSlider     = nullptr;
    shell::ui::Slider* m_brightnessSlider = nullptr;
    shell::ui::Label*  m_netLabel         = nullptr;
};

} // namespace shell::flyout
