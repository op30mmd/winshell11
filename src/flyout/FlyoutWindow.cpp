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

// ── Layout & palette ──────────────────────────────────────────────
static constexpr int PAD = 14;
static constexpr int TG_W = 104;
static constexpr int TG_H = 68;
static constexpr int SL_H = 30;
static constexpr int BTN_H = 36;
static constexpr int SEP = 1;

static constexpr COLORREF BG = RGB(28, 28, 34);
static constexpr COLORREF CARD = RGB(42, 42, 48);
static constexpr COLORREF TEXT_ON = RGB(220, 220, 225);
static constexpr COLORREF TEXT_DIM = RGB(140, 140, 150);
static constexpr COLORREF ACCENT = RGB(0, 120, 215);
static constexpr COLORREF ACCENT_HVR = RGB(20, 140, 240);
static constexpr COLORREF SEP_BG = RGB(50, 50, 55);
static constexpr COLORREF TG_BG = RGB(55, 55, 62);
static constexpr COLORREF SL_BG = RGB(50, 50, 56);
static constexpr COLORREF SL_FILL = RGB(0, 120, 215);

// ── Font helpers ──────────────────────────────────────────────────

static void DrawGlyph(HDC hdc, int x, int y, int w, int h, wchar_t glyph,
                      COLORREF col, int size) {
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, col);
    HFONT f = CreateFontW(size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe MDL2 Assets");
    HFONT old = (HFONT)SelectObject(hdc, f);
    wchar_t txt[2] = {glyph, 0};
    RECT rc = {x, y, x + w, y + h};
    DrawTextW(hdc, txt, 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, old);
    DeleteObject(f);
}

static void DrawLabel(HDC hdc, int x, int y, int w, int h,
                      const std::wstring& text, COLORREF col, int size,
                      UINT fmt = DT_LEFT | DT_VCENTER) {
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, col);
    HFONT f = CreateFontW(size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HFONT old = (HFONT)SelectObject(hdc, f);
    RECT rc = {x, y, x + w, y + h};
    DrawTextW(hdc, text.c_str(), -1, &rc, fmt);
    SelectObject(hdc, old);
    DeleteObject(f);
}

static void FillCol(HDC hdc, int x, int y, int w, int h, COLORREF col) {
    RECT rc = {x, y, x + w, y + h};
    HBRUSH br = CreateSolidBrush(col);
    FillRect(hdc, &rc, br);
    DeleteObject(br);
}

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
    return true;
}

void FlyoutWindow::Show(HWND parent, int /*tx*/, int /*ty*/, int /*tw*/, int /*th*/) {
    m_hwndParent = parent;

    m_volume = GetCurrentVolume();
    m_hasBrightness = HasBrightnessControl();
    m_brightness = m_hasBrightness ? GetCurrentBrightness() : -1;
    m_wifiOn = true; // default on

    // Recalculate height
    int h = PAD + TG_H + 8 + SEP + 8;
    if (m_hasBrightness) h += 16 + SL_H + 6;
    h += 16 + SL_H + 8 + SEP + 8;
    h += 48 + 8 + SEP + 8;
    h += BTN_H + PAD;
    m_height = h;

    // Position above the parent taskbar, right-aligned
    RECT prc;
    GetWindowRect(parent, &prc);
    int x = prc.right - m_width - 6;
    int y = prc.top - m_height - 6;
    // Clamp to screen
    if (y < 0) y = 0;

    SetWindowPos(m_hwnd, HWND_TOPMOST, x, y, m_width, m_height,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    SetFocus(parent);
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void FlyoutWindow::Dismiss() {
    if (m_hwnd && IsWindowVisible(m_hwnd))
        ShowWindow(m_hwnd, SW_HIDE);
}

// ── Hit testing ───────────────────────────────────────────────────

enum Sect {
    S_NONE = -1,
    S_WIFI, S_BT,
    S_BRIGHT, S_VOL,
    S_NET,
    S_LOCK, S_SLEEP, S_RESTART, S_SHUTDOWN,
};

int FlyoutWindow::HitTestSection(POINT pt) const {
    int y = PAD;
    // Toggles
    if (pt.y >= y && pt.y < y + TG_H) {
        if (pt.x >= PAD && pt.x < PAD + TG_W) return S_WIFI;
        if (pt.x >= PAD + TG_W + 8 && pt.x < PAD + TG_W * 2 + 8) return S_BT;
        return S_NONE;
    }
    y += TG_H + 8 + SEP + 8;

    // Brightness
    if (m_hasBrightness) {
        y += 16;
        if (pt.y >= y && pt.y < y + SL_H) return S_BRIGHT;
        y += SL_H + 6;
    }
    // Volume
    y += 16;
    if (pt.y >= y && pt.y < y + SL_H) return S_VOL;
    y += SL_H + 8 + SEP + 8;

    // Network
    if (pt.y >= y && pt.y < y + 48) return S_NET;
    y += 48 + 8 + SEP + 8;

    // Power row
    int pw = (m_width - PAD * 2 - 8) / 4;
    if (pt.y >= y && pt.y < y + BTN_H) {
        int px = PAD;
        for (int i = 0; i < 4; i++) {
            if (pt.x >= px && pt.x < px + pw) {
                switch (i) {
                    case 0: return S_LOCK;
                    case 1: return S_SLEEP;
                    case 2: return S_RESTART;
                    case 3: return S_SHUTDOWN;
                }
            }
            px += pw + 2;
        }
    }
    return S_NONE;
}

void FlyoutWindow::OnClick(int sect, HWND hwnd) {
    (void)hwnd;
    switch (sect) {
        case S_WIFI:   ToggleWiFi(); break;
        case S_BT:     ToggleBluetooth(); break;
        case S_LOCK:   shell::power::PowerManager::Lock(); Dismiss(); break;
        case S_SLEEP:  shell::power::PowerManager::Sleep(); break;
        case S_RESTART: shell::power::PowerManager::Restart(); break;
        case S_SHUTDOWN: shell::power::PowerManager::Shutdown(); break;
        default: break;
    }
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

// ── WiFi ──────────────────────────────────────────────────────────

std::wstring FlyoutWindow::GetWiFiStatus() {
    // Quick check: look for a network adapter with "Wireless" or "Wi-Fi" in the name
    ULONG len = sizeof(IP_ADAPTER_INFO) * 16;
    std::vector<BYTE> buf(len);
    PIP_ADAPTER_INFO ai = (PIP_ADAPTER_INFO)buf.data();
    if (GetAdaptersInfo(ai, &len) == ERROR_SUCCESS) {
        for (; ai; ai = ai->Next) {
            if (ai->Type == IF_TYPE_IEEE80211) {
                m_wifiOn = ai->IpAddressList.IpAddress.String[0] != '0';
                if (m_wifiOn) {
                    std::string d(ai->Description);
                    // Extract a short name
                    auto pos = d.find("Wi-Fi");
                    if (pos == std::string::npos) pos = d.find("Wireless");
                    if (pos != std::string::npos)
                        return L"Connected";
                    return L"Connected";
                }
                return L"Disconnected";
            }
        }
    }
    // No wireless adapter found
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
            method->Release();
        }
        cls->Release();
        svc->Release();
    }
    loc->Release();
    InvalidateRect(m_hwnd, nullptr, TRUE);
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
    InvalidateRect(m_hwnd, nullptr, TRUE);
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
//  Painting
// ═════════════════════════════════════════════════════════════════

void FlyoutWindow::OnPaint(HDC hdc) {
    int y = PAD;
    FillCol(hdc, 0, 0, m_width, m_height, BG);

    // ── Quick toggles ─────────────────────────────────────────────
    // Wi-Fi
    FillCol(hdc, PAD, y, TG_W, TG_H, m_wifiOn ? ACCENT : TG_BG);
    DrawGlyph(hdc, PAD, y + 6, TG_W, 30, 0xE701, TEXT_ON, 26);
    DrawLabel(hdc, PAD, y + TG_H - 22, TG_W, 18, L"Wi-Fi", TEXT_ON, 12, DT_CENTER);
    // Bluetooth
    int bx = PAD + TG_W + 8;
    FillCol(hdc, bx, y, TG_W, TG_H, m_btOn ? ACCENT : TG_BG);
    DrawGlyph(hdc, bx, y + 6, TG_W, 30, 0xE702, TEXT_ON, 26);
    DrawLabel(hdc, bx, y + TG_H - 22, TG_W, 18, L"Bluetooth", TEXT_ON, 12, DT_CENTER);
    y += TG_H + 8;

    FillCol(hdc, PAD, y, m_width - PAD * 2, SEP, SEP_BG);
    y += SEP + 8;

    // ── Brightness ────────────────────────────────────────────────
    if (m_hasBrightness) {
        DrawLabel(hdc, PAD, y, 120, 16, L"Brightness", TEXT_DIM, 12);
        y += 16;
        int sw = m_width - PAD * 2;
        FillCol(hdc, PAD, y, sw, SL_H, SL_BG);
        int fw = sw * m_brightness / 100;
        FillCol(hdc, PAD, y, fw, SL_H, SL_FILL);
        DrawGlyph(hdc, PAD + 4, y, SL_H, SL_H, 0xE706, TEXT_ON, 16);
        std::wstring bs = std::to_wstring(m_brightness) + L"%";
        DrawLabel(hdc, PAD + sw - 48, y, 44, SL_H, bs, TEXT_ON, 12, DT_RIGHT | DT_VCENTER);
        y += SL_H + 6;
    }

    // ── Volume ────────────────────────────────────────────────────
    DrawLabel(hdc, PAD, y, 120, 16, L"Volume", TEXT_DIM, 12);
    y += 16;
    {   int sw = m_width - PAD * 2;
        FillCol(hdc, PAD, y, sw, SL_H, SL_BG);
        int fw = sw * m_volume / 100;
        FillCol(hdc, PAD, y, fw, SL_H, SL_FILL);
        DrawGlyph(hdc, PAD + 4, y, SL_H, SL_H, 0xE995, TEXT_ON, 16);
        std::wstring vs = std::to_wstring(m_volume) + L"%";
        DrawLabel(hdc, PAD + sw - 48, y, 44, SL_H, vs, TEXT_ON, 12, DT_RIGHT | DT_VCENTER);
        y += SL_H + 8;
    }

    FillCol(hdc, PAD, y, m_width - PAD * 2, SEP, SEP_BG);
    y += SEP + 8;

    // ── Network info ──────────────────────────────────────────────
    std::wstring ni = GetNetworkInfo();
    DrawLabel(hdc, PAD, y, m_width - PAD * 2, 48, ni, TEXT_DIM, 11, DT_LEFT | DT_WORDBREAK);
    y += 48 + 8;

    FillCol(hdc, PAD, y, m_width - PAD * 2, SEP, SEP_BG);
    y += SEP + 8;

    // ── Power buttons ─────────────────────────────────────────────
    static const wchar_t* icons[] = {L"\uE72E", L"\uE73A", L"\uE777", L"\uE7E8"};
    static const wchar_t* labels[] = {L"Lock", L"Sleep", L"Restart", L"Shut down"};
    int pw = (m_width - PAD * 2 - 8) / 4;
    for (int i = 0; i < 4; i++) {
        int px = PAD + i * (pw + 2);
        FillCol(hdc, px, y, pw, BTN_H, CARD);
        DrawGlyph(hdc, px, y + 2, pw, 20, icons[i][0], TEXT_ON, 18);
        DrawLabel(hdc, px, y + 20, pw, 14, labels[i], TEXT_DIM, 10, DT_CENTER);
    }
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

    switch (msg) {
        case WM_ERASEBKGND: return 1;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd, &ps);
            HDC mem = CreateCompatibleDC(dc);
            HBITMAP bmp = CreateCompatibleBitmap(dc, p->m_width, p->m_height);
            HBITMAP old = (HBITMAP)SelectObject(mem, bmp);
            p->OnPaint(mem);
            BitBlt(dc, 0, 0, p->m_width, p->m_height, mem, 0, 0, SRCCOPY);
            SelectObject(mem, old);
            DeleteObject(bmp);
            DeleteDC(mem);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            int s = p->HitTestSection(pt);
            if (s == S_BRIGHT && p->m_hasBrightness) {
                p->m_draggingBrightness = true;
                int v = (pt.x - PAD) * 100 / (p->m_width - PAD * 2);
                p->SetBrightness(v);
            } else if (s == S_VOL) {
                p->m_draggingVolume = true;
                int v = (pt.x - PAD) * 100 / (p->m_width - PAD * 2);
                p->SetVolume(v);
            } else if (s >= 0) {
                p->OnClick(s, hwnd);
            }
            SetCapture(hwnd);
            return 0;
        }

        case WM_MOUSEMOVE:
            if (p->m_draggingBrightness && (w & MK_LBUTTON)) {
                POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
                int v = (pt.x - PAD) * 100 / (p->m_width - PAD * 2);
                p->SetBrightness(v);
            }
            if (p->m_draggingVolume && (w & MK_LBUTTON)) {
                POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
                int v = (pt.x - PAD) * 100 / (p->m_width - PAD * 2);
                p->SetVolume(v);
            }
            return 0;

        case WM_LBUTTONUP:
            ReleaseCapture();
            p->m_draggingBrightness = false;
            p->m_draggingVolume = false;
            return 0;

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
