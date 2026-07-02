#define SHELLBACKEND_EXPORTS
#include "exports.h"
#include <vector>
#include <string>

// ── Backend class headers ─────────────────────────────────────────
#include "../power/PowerManager.h"
#include "../desktop/IconManager.h"
#include "../launcher/AppLauncher.h"

// ── Window helpers (from FlyoutWindow.cpp) ────────────────────────
// These are extracted standalone versions of the flyout private helpers.

static int GetCurrentVolumeImpl() {
    DWORD vol;
    waveOutGetVolume(nullptr, &vol);
    WORD left = LOWORD(vol);
    WORD right = HIWORD(vol);
    WORD avg = (WORD)(((DWORD)left + (DWORD)right) / 2);
    return (int)(avg * 100.0f / 0xFFFF);
}

static void SetVolumeImpl(int val) {
    DWORD vol = (DWORD)(val * 0xFFFF / 100.0f);
    vol = vol | (vol << 16);
    waveOutSetVolume(nullptr, vol);
}

#include <wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")

static bool HasBrightnessControlImpl() {
    IWbemLocator* locator = nullptr;
    IWbemServices* services = nullptr;
    bool result = false;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) return false;

    hr = CoInitializeSecurity(nullptr, -1, nullptr, nullptr,
        RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr, EOAC_NONE, nullptr);
    if (FAILED(hr)) { CoUninitialize(); return false; }

    hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (void**)&locator);
    if (FAILED(hr)) { CoUninitialize(); return false; }

    BSTR ns = SysAllocString(L"root\\WMI");
    hr = locator->ConnectServer(ns, nullptr, nullptr, nullptr, 0, nullptr, nullptr, &services);
    SysFreeString(ns);
    if (FAILED(hr)) { locator->Release(); CoUninitialize(); return false; }

    BSTR wql = SysAllocString(L"WQL");
    BSTR query = SysAllocString(L"SELECT CurrentBrightness FROM WmiMonitorBrightness");
    IEnumWbemClassObject* enumerator = nullptr;
    hr = services->ExecQuery(wql, query,
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        nullptr, &enumerator);
    SysFreeString(wql);
    SysFreeString(query);

    if (SUCCEEDED(hr) && enumerator) {
        IWbemClassObject* obj = nullptr;
        ULONG ret = 0;
        if (enumerator->Next(WBEM_INFINITE, 1, &obj, &ret) == WBEM_S_NO_ERROR) {
            VARIANT v;
            VariantInit(&v);
            if (SUCCEEDED(obj->Get(L"CurrentBrightness", 0, &v, nullptr, nullptr))) {
                result = (v.vt == VT_I4 && v.uintVal > 0);
                VariantClear(&v);
            }
            obj->Release();
        }
        enumerator->Release();
    }

    services->Release();
    locator->Release();
    CoUninitialize();
    return result;
}

static int GetCurrentBrightnessImpl() {
    IWbemLocator* locator = nullptr;
    IWbemServices* services = nullptr;
    int brightness = 80;

    (void)CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    (void)CoInitializeSecurity(nullptr, -1, nullptr, nullptr,
        RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr, EOAC_NONE, nullptr);

    if (SUCCEEDED(CoCreateInstance(CLSID_WbemLocator, nullptr,
        CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&locator))) {
        BSTR ns = SysAllocString(L"root\\WMI");
        if (SUCCEEDED(locator->ConnectServer(ns, nullptr, nullptr, nullptr, 0,
            nullptr, nullptr, &services))) {
            BSTR wql = SysAllocString(L"WQL");
            BSTR query = SysAllocString(L"SELECT * FROM WmiMonitorBrightness");
            IEnumWbemClassObject* enumerator = nullptr;
            if (SUCCEEDED(services->ExecQuery(wql, query,
                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                nullptr, &enumerator))) {
                SysFreeString(wql);
                IWbemClassObject* obj = nullptr;
                ULONG ret = 0;
                if (enumerator->Next(WBEM_INFINITE, 1, &obj, &ret) == WBEM_S_NO_ERROR) {
                    VARIANT v;
                    VariantInit(&v);
                    if (SUCCEEDED(obj->Get(L"CurrentBrightness", 0, &v, nullptr, nullptr))) {
                        brightness = (int)v.uintVal;
                        VariantClear(&v);
                    }
                    obj->Release();
                }
                enumerator->Release();
            }
            SysFreeString(query);
            services->Release();
        }
        SysFreeString(ns);
        locator->Release();
    }
    CoUninitialize();
    return brightness;
}

static void SetBrightnessImpl(int val) {
    IWbemLocator* locator = nullptr;
    IWbemServices* services = nullptr;

    (void)CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    (void)CoInitializeSecurity(nullptr, -1, nullptr, nullptr,
        RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr, EOAC_NONE, nullptr);

    if (SUCCEEDED(CoCreateInstance(CLSID_WbemLocator, nullptr,
        CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&locator))) {
        BSTR ns = SysAllocString(L"root\\WMI");
        if (SUCCEEDED(locator->ConnectServer(ns, nullptr, nullptr, nullptr, 0,
            nullptr, nullptr, &services))) {
            BSTR methodName = SysAllocString(L"WmiSetBrightness");
            BSTR className = SysAllocString(L"WmiMonitorBrightnessMethods");
            IWbemClassObject* classObj = nullptr;
            if (SUCCEEDED(services->GetObject(className, 0, nullptr, &classObj, nullptr))) {
                IWbemClassObject* params = nullptr;
                classObj->GetMethod(methodName, 0, &params, nullptr);
                if (params) {
                    VARIANT v;
                    VariantInit(&v);
                    v.vt = VT_I4;
                    v.intVal = 50; // timeout
                    params->Put(L"Timeout", 0, &v, 0);
                    v.intVal = val;
                    params->Put(L"Brightness", 0, &v, 0);
                    services->ExecMethod(className, methodName, 0, nullptr, params, nullptr, nullptr);
                    params->Release();
                }
                classObj->Release();
            }
            SysFreeString(className);
            SysFreeString(methodName);
            services->Release();
        }
        SysFreeString(ns);
        locator->Release();
    }
    CoUninitialize();
}

#include <wlanapi.h>
#pragma comment(lib, "wlanapi.lib")

static void ToggleWiFiImpl() {
    HANDLE hClient = nullptr;
    DWORD dwMaxClient = 2;
    DWORD dwCurVersion = 0;
    DWORD dwResult = WlanOpenHandle(dwMaxClient, nullptr, &dwCurVersion, &hClient);
    if (dwResult != ERROR_SUCCESS) return;

    PWLAN_INTERFACE_INFO_LIST pList = nullptr;
    dwResult = WlanEnumInterfaces(hClient, nullptr, &pList);
    if (dwResult == ERROR_SUCCESS && pList && pList->dwNumberOfItems > 0) {
        GUID guid = pList->InterfaceInfo[0].InterfaceGuid;
        WLAN_INTF_OPCODE opCode = wlan_intf_opcode_current_operation_mode;
        WLAN_OPCODE_VALUE_TYPE opCodeType;
        DWORD dataSize = 0;
        PVOID data = nullptr;

        dwResult = WlanQueryInterface(hClient, &guid, opCode, nullptr, &dataSize, &data, &opCodeType);
        if (dwResult == ERROR_SUCCESS && data) {
            ULONG mode = *(PULONG)data;
            DOT11_SSID ssid = {0};
            bool enable = (mode == 0); // turn on if off
            WlanSetInterface(hClient, &guid, wlan_intf_opcode_current_operation_mode,
                sizeof(ULONG), enable ? &mode : &mode, nullptr);
            WlanFreeMemory(data);
        }
    }
    if (pList) WlanFreeMemory(pList);
    WlanCloseHandle(hClient, nullptr);
}

static void ToggleBluetoothImpl() {
    ShellExecuteW(nullptr, L"open", L"ms-settings:bluetooth", nullptr, nullptr, SW_SHOW);
}

#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

#include <bluetoothapis.h>
#pragma comment(lib, "Bthprops.lib")

// ── Global backend instances ──────────────────────────────────────
static shell::desktop::IconManager  g_iconManager;
static shell::launcher::AppLauncher g_appLauncher;
static std::vector<SB_WindowInfo>   g_windowCache;

// ═════════════════════════════════════════════════════════════════
//  Exported function implementations
// ═════════════════════════════════════════════════════════════════

void SB_Log(const char* message) {
    OutputDebugStringA(message);
    OutputDebugStringA("\n");
}

// ── Power ─────────────────────────────────────────────────────────
void SB_Power_Lock()     { shell::power::PowerManager::Lock(); }
void SB_Power_SignOut()  { shell::power::PowerManager::SignOut(); }
void SB_Power_Restart()  { shell::power::PowerManager::Restart(); }
void SB_Power_Shutdown() { shell::power::PowerManager::Shutdown(); }
void SB_Power_Sleep()    { shell::power::PowerManager::Sleep(); }

// ── Volume ────────────────────────────────────────────────────────
int  SB_Volume_Get()            { return GetCurrentVolumeImpl(); }
void SB_Volume_Set(int val)     { SetVolumeImpl(val); }

// ── Brightness ────────────────────────────────────────────────────
bool SB_Brightness_HasControl() { return HasBrightnessControlImpl(); }
int  SB_Brightness_Get()        { return GetCurrentBrightnessImpl(); }
void SB_Brightness_Set(int val) { SetBrightnessImpl(val); }

// ── Network ───────────────────────────────────────────────────────
bool SB_Network_GetInfo(wchar_t* buf, int bufLen) {
    ULONG len = sizeof(IP_ADAPTER_INFO) * 16;
    std::vector<BYTE> tmp(len);
    PIP_ADAPTER_INFO ai = (PIP_ADAPTER_INFO)tmp.data();
    if (GetAdaptersInfo(ai, &len) == ERROR_SUCCESS) {
        for (; ai; ai = ai->Next) {
            if ((ai->Type == MIB_IF_TYPE_ETHERNET || ai->Type == IF_TYPE_IEEE80211) &&
                ai->IpAddressList.IpAddress.String[0] != '0') {
                std::string type = (ai->Type == IF_TYPE_IEEE80211) ? "Wi-Fi" : "Ethernet";
                std::string ip(ai->IpAddressList.IpAddress.String);
                std::string mask(ai->IpAddressList.IpMask.String);
                std::wstring result = std::wstring(type.begin(), type.end()) + L"  " +
                    std::wstring(ip.begin(), ip.end()) + L" / " +
                    std::wstring(mask.begin(), mask.end());
                wcsncpy_s(buf, bufLen, result.c_str(), _TRUNCATE);
                return true;
            }
        }
    }
    wcsncpy_s(buf, bufLen, L"No network", _TRUNCATE);
    return false;
}

// ── Wi-Fi ─────────────────────────────────────────────────────────
void SB_WiFi_Toggle() { ToggleWiFiImpl(); }
bool SB_WiFi_IsEnabled() {
    HANDLE hClient = nullptr;
    DWORD dwMaxClient = 2;
    DWORD dwCurVersion = 0;
    DWORD dwResult = WlanOpenHandle(dwMaxClient, nullptr, &dwCurVersion, &hClient);
    if (dwResult != ERROR_SUCCESS) return false;

    PWLAN_INTERFACE_INFO_LIST pList = nullptr;
    dwResult = WlanEnumInterfaces(hClient, nullptr, &pList);
    bool enabled = false;
    if (dwResult == ERROR_SUCCESS && pList && pList->dwNumberOfItems > 0) {
        GUID guid = pList->InterfaceInfo[0].InterfaceGuid;
        WLAN_OPCODE_VALUE_TYPE opCodeType;
        DWORD dataSize = 0;
        ULONG mode = 0;
        dwResult = WlanQueryInterface(hClient, &guid, wlan_intf_opcode_current_operation_mode,
            nullptr, &dataSize, (PVOID*)&mode, &opCodeType);
        if (dwResult == ERROR_SUCCESS)
            enabled = (mode != 0);
    }
    if (pList) WlanFreeMemory(pList);
    WlanCloseHandle(hClient, nullptr);
    return enabled;
}

// ── Bluetooth ─────────────────────────────────────────────────────
void SB_Bluetooth_Toggle() { ToggleBluetoothImpl(); }
bool SB_Bluetooth_HasHardware() {
    BLUETOOTH_FIND_RADIO_PARAMS btParams = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };
    HANDLE hRadio = nullptr;
    HBLUETOOTH_RADIO_FIND hFind = BluetoothFindFirstRadio(&btParams, &hRadio);
    if (!hFind) return false;
    CloseHandle(hRadio);
    BluetoothFindRadioClose(hFind);
    return true;
}

bool SB_Bluetooth_IsEnabled() {
    BLUETOOTH_FIND_RADIO_PARAMS btParams = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };
    HANDLE hRadio = nullptr;
    HBLUETOOTH_RADIO_FIND hFind = BluetoothFindFirstRadio(&btParams, &hRadio);
    if (!hFind) return false;
    CloseHandle(hRadio);
    BluetoothFindRadioClose(hFind);

    // Check registry for BTHPORT enabled state
    HKEY hKey;
    bool enabled = true; // default: enabled if hardware exists
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Services\\BTHPORT\\Parameters",
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD val = 0;
        DWORD type = 0;
        DWORD cb = sizeof(val);
        if (RegQueryValueExW(hKey, L"IsEnabled", nullptr, &type, (LPBYTE)&val, &cb) == ERROR_SUCCESS)
            enabled = (val != 0);
        RegCloseKey(hKey);
    }
    return enabled;
}

// ── Desktop Icons ─────────────────────────────────────────────────
int SB_Desktop_EnumerateIcons(SB_DesktopIcon* icons, int maxIcons) {
    g_iconManager.FreeIcons();
    g_iconManager.EnumerateIcons();
    const auto& items = g_iconManager.GetIcons();
    if (!icons || maxIcons <= 0)
        return (int)items.size();
    int count = min((int)items.size(), maxIcons);
    for (int i = 0; i < count; i++) {
        wcsncpy_s(icons[i].name, 256, items[i].name.c_str(), _TRUNCATE);
        wcsncpy_s(icons[i].path, 1024, items[i].path.c_str(), _TRUNCATE);
        wcsncpy_s(icons[i].parsingName, 1024, items[i].parsingName.c_str(), _TRUNCATE);
        icons[i].x = items[i].x;
        icons[i].y = items[i].y;
    }
    return count;
}

void SB_Desktop_FreeIcons() {
    g_iconManager.FreeIcons();
}

// ── App Launcher ──────────────────────────────────────────────────
int SB_Launcher_Enumerate(SB_AppItem* items, int maxItems) {
    static bool enumerated = false;
    if (!enumerated) {
        g_appLauncher.EnumerateApps();
        enumerated = true;
    }
    const auto& apps = g_appLauncher.GetApps();
    if (!items || maxItems <= 0)
        return (int)apps.size();
    int count = min((int)apps.size(), maxItems);
    for (int i = 0; i < count; i++) {
        wcsncpy_s(items[i].name, 256, apps[i].name.c_str(), _TRUNCATE);
        wcsncpy_s(items[i].path, 1024, apps[i].path.c_str(), _TRUNCATE);
        wcsncpy_s(items[i].target, 1024, apps[i].target.c_str(), _TRUNCATE);
        items[i].type = apps[i].type;
    }
    return count;
}

void SB_Launcher_Launch(const wchar_t* path) {
    shell::launcher::AppItem item;
    item.path = path;
    g_appLauncher.Launch(item);
}

// ── Window Watcher ────────────────────────────────────────────────
bool SB_Watcher_Start() {
    // Simplified: enumerate visible windows once.
    g_windowCache.clear();
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        if (!IsWindowVisible(hwnd)) return TRUE;
        WCHAR title[512];
        GetWindowTextW(hwnd, title, 512);
        if (title[0] == 0) return TRUE;
        auto& cache = *(std::vector<SB_WindowInfo>*)lParam;
        SB_WindowInfo info;
        info.hwnd = hwnd;
        wcsncpy_s(info.title, 512, title, _TRUNCATE);
        cache.push_back(info);
        return TRUE;
    }, (LPARAM)&g_windowCache);
    return true;
}

void SB_Watcher_Stop() {
    g_windowCache.clear();
}

int SB_Watcher_GetWindows(SB_WindowInfo* buf, int maxWindows) {
    int count = min((int)g_windowCache.size(), maxWindows);
    for (int i = 0; i < count; i++) {
        buf[i].hwnd = g_windowCache[i].hwnd;
        wcsncpy_s(buf[i].title, 512, g_windowCache[i].title, _TRUNCATE);
    }
    return count;
}

// ── Tray Host ─────────────────────────────────────────────────────
int SB_Tray_GetIconCount()       { return 0; } // TrayHost needs HWND - implemented in C# side
int SB_Tray_GetDesiredWidth()    { return 0; }

// ── Config ────────────────────────────────────────────────────────
bool SB_Config_GetWallpaperPath(wchar_t* buf, int bufLen) {
    if (!buf || bufLen <= 0) return false;
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD type = 0;
        DWORD size = bufLen * sizeof(wchar_t);
        LSTATUS status = RegQueryValueExW(hKey, L"Wallpaper", nullptr, &type, (LPBYTE)buf, &size);
        RegCloseKey(hKey);
        return status == ERROR_SUCCESS && buf[0] != 0;
    }
    return false;
}
