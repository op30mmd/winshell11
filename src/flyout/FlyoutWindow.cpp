#include "FlyoutWindow.h"
#include "common/Logger.h"
#include "power/PowerManager.h"

#include <iphlpapi.h>
#include <mmsystem.h>
#include <wlanapi.h>
#include <windowsx.h>
#include <wbemidl.h>
#include <vector>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "Wlanapi.lib")
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Winmm.lib")

namespace shell::flyout {

// ── WMI helper ────────────────────────────────────────────────────

template<typename Fn>
static bool WmiExec(const std::wstring& query, Fn cb) {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool needUninit = SUCCEEDED(hr) && hr != S_FALSE;
    IWbemLocator* loc = nullptr;
    IWbemServices* svc = nullptr;
    IEnumWbemClassObject* enumer = nullptr;
    bool ok = false;

    if (SUCCEEDED(CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_IWbemLocator, (void**)&loc)) &&
        SUCCEEDED(loc->ConnectServer(BSTR(L"ROOT\\WMI"), nullptr, nullptr, nullptr,
                                     0, nullptr, nullptr, &svc)) &&
        SUCCEEDED(svc->ExecQuery(BSTR(L"WQL"), BSTR(query.c_str()),
                                 WBEM_FLAG_FORWARD_ONLY, nullptr, &enumer))) {
        IWbemClassObject* obj = nullptr;
        ULONG ret = 0;
        while (enumer->Next(WBEM_INFINITE, 1, &obj, &ret) == S_OK && ret) {
            cb(obj);
            obj->Release();
            ok = true;
        }
    }
    if (enumer) enumer->Release();
    if (svc) svc->Release();
    if (loc) loc->Release();
    if (needUninit) CoUninitialize();
    return ok;
}

// ═════════════════════════════════════════════════════════════════
//  FlyoutWindow implementation
// ═════════════════════════════════════════════════════════════════

bool FlyoutWindow::Create(HINSTANCE hInstance) {
    const wchar_t CLASS[] = L"ShellFlyoutClass";
    WNDCLASSW wc = {};
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS;
    wc.hbrBackground = nullptr;
    RegisterClassW(&wc);

    m_hwnd = CreateWindowExW(
        WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        CLASS, L"Quick Settings", WS_POPUP,
        0, 0, m_width, m_height,
        nullptr, nullptr, hInstance, this);

    if (!m_hwnd) return false;
    SetLayeredWindowAttributes(m_hwnd, 0, 235, LWA_ALPHA);

    // Attach the UI host and configure the theme to match the flyout palette.
    m_ui.Attach(m_hwnd);
    auto& theme          = m_ui.GetTheme();
    theme.background     = RGB(28, 28, 34);
    theme.surface        = RGB(42, 42, 48);
    theme.surfaceHover   = RGB(55, 55, 62);
    theme.surfacePressed = RGB(70, 70, 78);
    theme.accent         = RGB(0, 120, 215);
    theme.text           = RGB(220, 220, 225);
    theme.textSecondary  = RGB(140, 140, 150);
    theme.border         = RGB(50, 50, 55);
    theme.cornerRadius   = 6;
    theme.fontSize       = 13;
    theme.fontFamily     = L"Segoe UI";

    return true;
}

// ── Widget tree construction ──────────────────────────────────────

void FlyoutWindow::RebuildWidgetTree() {
    // Reset cached widget pointers — they will be re-assigned below.
    m_wifiLabel        = nullptr;
    m_btLabel          = nullptr;
    m_volumeSlider     = nullptr;
    m_brightnessSlider = nullptr;
    m_netLabel         = nullptr;

    // Root: vertical stack that fills the window.
    auto root = std::make_unique<shell::ui::StackPanel>();
    root->SetOrientation(shell::ui::Orientation::Vertical);
    root->SetSpacing(0);
    root->SetPadding(0);

    // ── Quick toggles (Wi-Fi / Bluetooth) ────────────────────────
    {
        auto row = std::make_unique<shell::ui::StackPanel>();
        row->SetOrientation(shell::ui::Orientation::Horizontal);
        row->SetSpacing(8);
        row->SetPadding(14);
        row->SetPreferredSize(0, 96); // 14 pad + 68 tile + 14 pad

        auto wifi = std::make_unique<shell::ui::Button>();
        wifi->SetText(m_wifiOn ? L"Wi-Fi\nOn" : L"Wi-Fi\nOff");
        wifi->SetPreferredSize(104, 68);
        row->AddChild(std::move(wifi));

        auto bt = std::make_unique<shell::ui::Button>();
        bt->SetText(m_btOn ? L"Bluetooth\nOn" : L"Bluetooth\nOff");
        bt->SetPreferredSize(104, 68);
        row->AddChild(std::move(bt));

        // Wire up click handlers after AddChild (pointers are stable).
        auto* wifiBtn = static_cast<shell::ui::Button*>(row->Children()[0].get());
        auto* btBtn   = static_cast<shell::ui::Button*>(row->Children()[1].get());

        wifiBtn->SetOnClick([this, wifiBtn]() {
            ToggleWiFi();
            wifiBtn->SetText(m_wifiOn ? L"Wi-Fi\nOn" : L"Wi-Fi\nOff");
        });
        btBtn->SetOnClick([this, btBtn]() {
            ToggleBluetooth();
            btBtn->SetText(m_btOn ? L"Bluetooth\nOn" : L"Bluetooth\nOff");
        });

        root->AddChild(std::move(row));
    }

    // Separator
    {
        auto sep = std::make_unique<shell::ui::Widget>();
        sep->SetPreferredSize(0, 1);
        root->AddChild(std::move(sep));
    }

    // ── Brightness (optional) ─────────────────────────────────────
    if (m_hasBrightness) {
        auto section = std::make_unique<shell::ui::StackPanel>();
        section->SetOrientation(shell::ui::Orientation::Vertical);
        section->SetSpacing(4);
        section->SetPadding(14);
        section->SetPreferredSize(0, 78); // 14 + 16 + 4 + 30 + 14

        auto lbl = std::make_unique<shell::ui::Label>();
        lbl->SetText(L"Brightness");
        lbl->SetSecondary(true);
        lbl->SetPreferredSize(0, 16);
        section->AddChild(std::move(lbl));

        auto slider = std::make_unique<shell::ui::Slider>();
        slider->SetValue(m_brightness);
        slider->SetPreferredSize(0, 30);
        slider->SetOnChange([this](int v) { SetBrightness(v); });
        m_brightnessSlider = static_cast<shell::ui::Slider*>(
            section->AddChild(std::move(slider)));

        root->AddChild(std::move(section));

        auto sep = std::make_unique<shell::ui::Widget>();
        sep->SetPreferredSize(0, 1);
        root->AddChild(std::move(sep));
    }

    // ── Volume ────────────────────────────────────────────────────
    {
        auto section = std::make_unique<shell::ui::StackPanel>();
        section->SetOrientation(shell::ui::Orientation::Vertical);
        section->SetSpacing(4);
        section->SetPadding(14);
        section->SetPreferredSize(0, 78); // 14 + 16 + 4 + 30 + 14

        auto lbl = std::make_unique<shell::ui::Label>();
        lbl->SetText(L"Volume");
        lbl->SetSecondary(true);
        lbl->SetPreferredSize(0, 16);
        section->AddChild(std::move(lbl));

        auto slider = std::make_unique<shell::ui::Slider>();
        slider->SetValue(m_volume);
        slider->SetPreferredSize(0, 30);
        slider->SetOnChange([this](int v) { SetVolume(v); });
        m_volumeSlider = static_cast<shell::ui::Slider*>(
            section->AddChild(std::move(slider)));

        root->AddChild(std::move(section));
    }

    // Separator
    {
        auto sep = std::make_unique<shell::ui::Widget>();
        sep->SetPreferredSize(0, 1);
        root->AddChild(std::move(sep));
    }

    // ── Network info ──────────────────────────────────────────────
    {
        auto section = std::make_unique<shell::ui::StackPanel>();
        section->SetOrientation(shell::ui::Orientation::Vertical);
        section->SetSpacing(0);
        section->SetPadding(14);
        section->SetPreferredSize(0, 76); // 14 + 48 + 14

        auto lbl = std::make_unique<shell::ui::Label>();
        lbl->SetText(GetNetworkInfo());
        lbl->SetSecondary(true);
        lbl->SetAlignment(DT_LEFT | DT_WORDBREAK);
        lbl->SetPreferredSize(0, 48);
        m_netLabel = static_cast<shell::ui::Label*>(
            section->AddChild(std::move(lbl)));

        root->AddChild(std::move(section));
    }

    // Separator
    {
        auto sep = std::make_unique<shell::ui::Widget>();
        sep->SetPreferredSize(0, 1);
        root->AddChild(std::move(sep));
    }

    // ── Power buttons ─────────────────────────────────────────────
    {
        auto row = std::make_unique<shell::ui::StackPanel>();
        row->SetOrientation(shell::ui::Orientation::Horizontal);
        row->SetSpacing(4);
        row->SetPadding(14);
        row->SetPreferredSize(0, 64); // 14 + 36 + 14

        struct PowerBtn { const wchar_t* label; std::function<void()> action; };
        PowerBtn btns[] = {
            { L"Lock",      [this]() { shell::power::PowerManager::Lock();     Dismiss(); } },
            { L"Sleep",     []()     { shell::power::PowerManager::Sleep();              } },
            { L"Restart",   []()     { shell::power::PowerManager::Restart();            } },
            { L"Shut down", []()     { shell::power::PowerManager::Shutdown();           } },
        };

        for (auto& pb : btns) {
            auto btn = std::make_unique<shell::ui::Button>();
            btn->SetText(pb.label);
            btn->SetPreferredSize(0, 36); // width=0 → stretch equally via StackPanel
            btn->SetOnClick(pb.action);
            row->AddChild(std::move(btn));
        }

        root->AddChild(std::move(row));
    }

    m_ui.SetRoot(std::move(root));
}

void FlyoutWindow::Show(HWND parent, int /*tx*/, int /*ty*/, int /*tw*/, int /*th*/) {
    m_hwndParent = parent;

    m_volume        = GetCurrentVolume();
    m_hasBrightness = HasBrightnessControl();
    m_brightness    = m_hasBrightness ? GetCurrentBrightness() : 80;
    m_wifiOn        = true; // default on

    // Recalculate window height based on content sections.
    static constexpr int PAD   = 14;
    static constexpr int TG_H  = 68;
    static constexpr int SL_H  = 30;
    static constexpr int BTN_H = 36;

    int h = (PAD + TG_H + PAD) + 1;          // toggles + separator
    if (m_hasBrightness)
        h += (PAD + 16 + 4 + SL_H + PAD) + 1; // brightness + separator
    h += (PAD + 16 + 4 + SL_H + PAD) + 1;    // volume + separator
    h += (PAD + 48 + PAD) + 1;               // network + separator
    h += (PAD + BTN_H + PAD);                // power
    m_height = h;

    // Position above the parent taskbar, right-aligned.
    RECT prc;
    GetWindowRect(parent, &prc);
    int x = prc.right - m_width - 6;
    int y = prc.top - m_height - 6;
    if (y < 0) y = 0;

    SetWindowPos(m_hwnd, HWND_TOPMOST, x, y, m_width, m_height,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    SetFocus(parent);

    // Rebuild the widget tree with fresh state, then layout.
    RebuildWidgetTree();
}

void FlyoutWindow::Dismiss() {
    if (m_hwnd && IsWindowVisible(m_hwnd))
        ShowWindow(m_hwnd, SW_HIDE);
}

// ── WiFi ──────────────────────────────────────────────────────────

std::wstring FlyoutWindow::GetWiFiStatus() {
    ULONG len = sizeof(IP_ADAPTER_INFO) * 16;
    std::vector<BYTE> buf(len);
    PIP_ADAPTER_INFO ai = (PIP_ADAPTER_INFO)buf.data();
    if (GetAdaptersInfo(ai, &len) == ERROR_SUCCESS) {
        for (; ai; ai = ai->Next) {
            if (ai->Type == IF_TYPE_IEEE80211) {
                m_wifiOn = ai->IpAddressList.IpAddress.String[0] != '0';
                return m_wifiOn ? L"Connected" : L"Disconnected";
            }
        }
    }
    m_wifiOn = false;
    return L"Off";
}

void FlyoutWindow::ToggleWiFi() {
    HANDLE hWlan = nullptr;
    DWORD ver = 0;
    if (WlanOpenHandle(2, nullptr, &ver, &hWlan) != ERROR_SUCCESS) {
        ShellExecuteW(nullptr, L"open", L"ms-settings:network-wifi", nullptr, nullptr, SW_SHOWNORMAL);
        m_wifiOn = !m_wifiOn;
        return;
    }
    PWLAN_INTERFACE_INFO_LIST ifList = nullptr;
    if (WlanEnumInterfaces(hWlan, nullptr, &ifList) == ERROR_SUCCESS) {
        for (DWORD i = 0; i < ifList->dwNumberOfItems; i++) {
            GUID guid = ifList->InterfaceInfo[i].InterfaceGuid;
            // Read current radio state
            PWLAN_RADIO_STATE radioState = nullptr;
            DWORD rsSize = 0;
            WLAN_OPCODE_VALUE_TYPE opCode;
            if (WlanQueryInterface(hWlan, &guid, wlan_intf_opcode_radio_state,
                                   nullptr, &rsSize, (PVOID*)&radioState, &opCode) == ERROR_SUCCESS && radioState) {
                DOT11_RADIO_STATE newState = m_wifiOn ? dot11_radio_state_off : dot11_radio_state_on;
                // Set the software radio state via WlanSetInterface
                WLAN_PHY_RADIO_STATE phyState;
                phyState.dwPhyIndex = 0;
                phyState.dot11SoftwareRadioState = newState;
                WlanSetInterface(hWlan, &guid, wlan_intf_opcode_radio_state,
                                 sizeof(WLAN_PHY_RADIO_STATE), &phyState, nullptr);
                WlanFreeMemory(radioState);
            }
        }
        WlanFreeMemory(ifList);
    }
    WlanCloseHandle(hWlan, nullptr);
    m_wifiOn = !m_wifiOn;
}

// ── Bluetooth ─────────────────────────────────────────────────────

std::wstring FlyoutWindow::GetBluetoothStatus() {
    // Quick check via registry for Bluetooth radios
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\BTHPORT\\Parameters",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        m_btOn = true;
        return L"On";
    }
    m_btOn = false;
    return L"Off";
}

void FlyoutWindow::ToggleBluetooth() {
    ShellExecuteW(nullptr, L"open", L"ms-settings:bluetooth", nullptr, nullptr, SW_SHOWNORMAL);
    m_btOn = !m_btOn;
}

// ── Brightness ────────────────────────────────────────────────────

bool FlyoutWindow::HasBrightnessControl() {
    bool found = false;
    WmiExec(L"SELECT * FROM WmiMonitorBrightness",
            [&](IWbemClassObject*) { found = true; });
    return found;
}

int FlyoutWindow::GetCurrentBrightness() {
    int val = 80;
    WmiExec(L"SELECT * FROM WmiMonitorBrightness",
            [&](IWbemClassObject* obj) {
                VARIANT v;
                if (SUCCEEDED(obj->Get(L"CurrentBrightness", 0, &v, nullptr, nullptr))) {
                    val = v.uintVal;
                    VariantClear(&v);
                }
            });
    return val;
}

void FlyoutWindow::SetBrightness(int val) {
    m_brightness = (std::max)(0, (std::min)(100, val));
    IWbemLocator* loc = nullptr;
    IWbemServices* svc = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_IWbemLocator, (void**)&loc)) &&
        SUCCEEDED(loc->ConnectServer(BSTR(L"ROOT\\WMI"), nullptr, nullptr, nullptr,
                                     0, nullptr, nullptr, &svc))) {
        IWbemClassObject* cls = nullptr;
        IWbemClassObject* method = nullptr;
        if (SUCCEEDED(svc->GetObject(BSTR(L"WmiSetBrightness"), 0, nullptr, &cls, nullptr)) &&
            SUCCEEDED(cls->GetMethod(L"SetBrightness", 0, &method, nullptr))) {
            IWbemClassObject* inP = nullptr;
            if (SUCCEEDED(method->SpawnInstance(0, &inP))) {
                VARIANT v;
                v.vt = VT_I4;
                v.intVal = m_brightness;
                inP->Put(L"Brightness", 0, &v, 0);
                v.intVal = 0;
                inP->Put(L"Timeout", 0, &v, 0);
                svc->ExecMethod(BSTR(L"WmiSetBrightness"), BSTR(L"SetBrightness"),
                                0, nullptr, inP, nullptr, nullptr);
                inP->Release();
            }
            if (method) method->Release();
        }
        if (cls) cls->Release();
        svc->Release();
    }
    if (loc) loc->Release();
    // Repaint is triggered by the Slider widget via Widget::Invalidate().
}

// ── Volume ────────────────────────────────────────────────────────

int FlyoutWindow::GetCurrentVolume() {
    DWORD vol = 0;
    if (waveOutGetVolume(nullptr, &vol) == MMSYSERR_NOERROR)
        return (int)((vol & 0xFFFF) * 100 / 0xFFFF);
    return 50;
}

void FlyoutWindow::SetVolume(int val) {
    m_volume = (std::max)(0, (std::min)(100, val));
    DWORD s = (DWORD)(m_volume * 0xFFFF / 100);
    waveOutSetVolume(nullptr, MAKELONG(s, s));
    // Repaint is triggered by the Slider widget via Widget::Invalidate().
}

// ── Network info ──────────────────────────────────────────────────

std::wstring FlyoutWindow::GetNetworkInfo() {
    ULONG len = sizeof(IP_ADAPTER_INFO) * 16;
    std::vector<BYTE> buf(len);
    PIP_ADAPTER_INFO ai = (PIP_ADAPTER_INFO)buf.data();
    if (GetAdaptersInfo(ai, &len) == ERROR_SUCCESS) {
        for (; ai; ai = ai->Next) {
            if ((ai->Type == MIB_IF_TYPE_ETHERNET || ai->Type == IF_TYPE_IEEE80211) &&
                ai->IpAddressList.IpAddress.String[0] != '0') {
                std::string type = (ai->Type == IF_TYPE_IEEE80211) ? "Wi-Fi" : "Ethernet";
                std::string ip(ai->IpAddressList.IpAddress.String);
                std::string mask(ai->IpAddressList.IpMask.String);
                auto wtype = std::wstring(type.begin(), type.end());
                auto wip = std::wstring(ip.begin(), ip.end());
                auto wmask = std::wstring(mask.begin(), mask.end());
                return wtype + L"  IP: " + wip + L"\nMask: " + wmask;
            }
        }
    }
    return L"No network";
}

// ═════════════════════════════════════════════════════════════════
//  Window procedure
// ═════════════════════════════════════════════════════════════════

LRESULT CALLBACK FlyoutWindow::WindowProc(HWND hwnd, UINT msg, WPARAM w, LPARAM lp) {
    FlyoutWindow* p = nullptr;
    if (msg == WM_NCCREATE) {
        p = (FlyoutWindow*)((CREATESTRUCT*)lp)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)p);
        return DefWindowProcW(hwnd, msg, w, lp);
    }
    p = (FlyoutWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (!p) return DefWindowProcW(hwnd, msg, w, lp);

    // Route all paint, input, and layout messages through the UI host.
    LRESULT result = 0;
    if (p->m_ui.HandleMessage(msg, w, lp, result))
        return result;

    switch (msg) {
        case WM_ACTIVATE:
            if (LOWORD(w) == WA_INACTIVE) p->Dismiss();
            return 0;

        case WM_KEYDOWN:
            if (w == VK_ESCAPE) p->Dismiss();
            return 0;

        case WM_SYSCOMMAND:
            if (w == SC_MINIMIZE || w == SC_CLOSE) return 0;
            return DefWindowProcW(hwnd, msg, w, lp);

        case WM_CLOSE:
            return 0;
    }
    return DefWindowProcW(hwnd, msg, w, lp);
}

} // namespace shell::flyout
