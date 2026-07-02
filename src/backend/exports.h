#pragma once
#include <windows.h>

#ifdef SHELLBACKEND_EXPORTS
#define SHELLBACKEND_API __declspec(dllexport)
#else
#define SHELLBACKEND_API __declspec(dllimport)
#endif

extern "C" {

// ── Logging ───────────────────────────────────────────────────────
SHELLBACKEND_API void SB_Log(const char* message);

// ── Power ─────────────────────────────────────────────────────────
SHELLBACKEND_API void SB_Power_Lock();
SHELLBACKEND_API void SB_Power_SignOut();
SHELLBACKEND_API void SB_Power_Restart();
SHELLBACKEND_API void SB_Power_Shutdown();
SHELLBACKEND_API void SB_Power_Sleep();

// ── Volume ────────────────────────────────────────────────────────
SHELLBACKEND_API int  SB_Volume_Get();
SHELLBACKEND_API void SB_Volume_Set(int val);

// ── Brightness ────────────────────────────────────────────────────
SHELLBACKEND_API bool SB_Brightness_HasControl();
SHELLBACKEND_API int  SB_Brightness_Get();
SHELLBACKEND_API void SB_Brightness_Set(int val);

// ── Network ───────────────────────────────────────────────────────
SHELLBACKEND_API bool SB_Network_GetInfo(wchar_t* buf, int bufLen);

// ── Wi-Fi ─────────────────────────────────────────────────────────
SHELLBACKEND_API void SB_WiFi_Toggle();
SHELLBACKEND_API bool SB_WiFi_IsEnabled();

// ── Bluetooth ─────────────────────────────────────────────────────
SHELLBACKEND_API void SB_Bluetooth_Toggle();
SHELLBACKEND_API bool SB_Bluetooth_IsEnabled();
SHELLBACKEND_API bool SB_Bluetooth_HasHardware();

// ── Desktop Icons ─────────────────────────────────────────────────
typedef struct {
    wchar_t name[256];
    wchar_t path[1024];
    wchar_t parsingName[1024];
    int x, y;
} SB_DesktopIcon;

SHELLBACKEND_API int  SB_Desktop_EnumerateIcons(SB_DesktopIcon* icons, int maxIcons);
SHELLBACKEND_API void SB_Desktop_FreeIcons();

// ── App Launcher ──────────────────────────────────────────────────
typedef struct {
    wchar_t name[256];
    wchar_t path[1024];
    wchar_t target[1024];
    int type;
} SB_AppItem;

SHELLBACKEND_API int  SB_Launcher_Enumerate(SB_AppItem* items, int maxItems);
SHELLBACKEND_API void SB_Launcher_Launch(const wchar_t* path);

// ── Window Watcher ────────────────────────────────────────────────
typedef struct {
    HWND hwnd;
    wchar_t title[512];
} SB_WindowInfo;

SHELLBACKEND_API bool SB_Watcher_Start();
SHELLBACKEND_API void SB_Watcher_Stop();
SHELLBACKEND_API int  SB_Watcher_GetWindows(SB_WindowInfo* buf, int maxWindows);

// ── Tray Host ─────────────────────────────────────────────────────
SHELLBACKEND_API int  SB_Tray_GetIconCount();
SHELLBACKEND_API int  SB_Tray_GetDesiredWidth();

// ── Config ────────────────────────────────────────────────────────
SHELLBACKEND_API bool SB_Config_GetWallpaperPath(wchar_t* buf, int bufLen);

} // extern "C"
