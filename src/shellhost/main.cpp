#include "MainWindow.h"
#include "HotkeyManager.h"
#include "common/Config.h"
#include "common/Logger.h"
#include "desktop/DesktopWindow.h"
#include "desktop/IconManager.h"
#include "flyout/FlyoutWindow.h"
#include "taskbar/TaskbarWindow.h"
#include "tray/TrayHost.h"
#include "launcher/AppLauncher.h"
#include "windowwatcher/WindowWatcher.h"
#include "power/PowerManager.h"

#include <gdiplus.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <windows.h>

#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

#pragma warning(push)
#pragma warning(disable : 28251)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/) {
#pragma warning(pop)
    shell::common::Logger::Initialize("shellhost");
    LOG_INFO("Shell starting...");

    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))) {
        LOG_ERROR("Failed to initialize COM");
        return 1;
    }

    GdiplusStartupInput gdiplusInput;
    ULONG_PTR gdiplusToken = 0;
    GdiplusStartup(&gdiplusToken, &gdiplusInput, nullptr);

    auto config = shell::common::Config::Load();

    // Message-only hub window
    shell::host::MainWindow mainWnd;
    if (!mainWnd.Create(hInstance)) {
        LOG_ERROR("Failed to create main window");
        GdiplusShutdown(gdiplusToken);
        CoUninitialize();
        return 1;
    }
    HWND hMain = mainWnd.GetHWND();

    // Desktop (full-screen background with wallpaper)
    shell::desktop::DesktopWindow desktop;
    if (!desktop.Create(hInstance, config.desktop)) {
        LOG_WARNING("Failed to create desktop window");
    }

    // Enumerate desktop icons and set on desktop window
    {
        shell::desktop::IconManager iconMgr;
        iconMgr.EnumerateIcons();
        std::vector<shell::desktop::DesktopIconData> desktopIcons;
        {
            const auto& icons = iconMgr.GetIcons();
            desktopIcons.reserve(icons.size());
            for (const auto& icon : icons) {
                HICON hIcon = nullptr;
                SHFILEINFOW sfi = {};
                // Use PIDL-based icon extraction (works for all items)
                if (icon.pidl &&
                    SHGetFileInfoW((LPCWSTR)icon.pidl, 0, &sfi, sizeof(sfi),
                                   SHGFI_PIDL | SHGFI_ICON | SHGFI_LARGEICON) &&
                    sfi.hIcon) {
                    hIcon = sfi.hIcon;
                }
                LPITEMIDLIST pidlCopy = icon.pidl ? ILClone(icon.pidl) : nullptr;
                desktopIcons.push_back({icon.name, icon.path, icon.x, icon.y, hIcon, pidlCopy, icon.parsingName});
            }
        }
        desktop.SetIcons(desktopIcons);
        iconMgr.FreeIcons();
    }

    // Taskbar (appbar at bottom)
    shell::taskbar::TaskbarWindow taskbar;
    if (!taskbar.Create(hInstance, config.taskbar)) {
        LOG_WARNING("Failed to create taskbar window");
    }
    HWND hTaskbar = taskbar.GetHWND();

    // Tray notification area (icon manager, no window)
    shell::tray::TrayHost tray;
    if (hTaskbar) {
        taskbar.SetTrayHost(&tray);
    }

    // Flyout control center (quick settings, power, etc.)
    shell::flyout::FlyoutWindow flyout;
    if (flyout.Create(hInstance)) {
        taskbar.SetFlyout(&flyout);
    }

    // Window watcher (tracks running windows for taskbar buttons)
    shell::watcher::WindowWatcher watcher;
    watcher.Start(
        [&taskbar, &watcher]() {
            taskbar.SetWindows(watcher.GetWindows());
        },
        [hTaskbar]() {
            if (hTaskbar)
                InvalidateRect(hTaskbar, nullptr, TRUE);
        });

    // Launcher (enumerate Start Menu apps)
    shell::launcher::AppLauncher launcher;
    launcher.EnumerateApps();
    if (hTaskbar) {
        taskbar.SetStartMenuCallback([&launcher, hTaskbar]() {
            launcher.ShowMenu(hTaskbar);
        });
    }

    // Hotkeys
    shell::host::HotkeyManager hotkeys;
    hotkeys.Initialize(hMain);
    mainWnd.SetHotkeyHandler([&](int id) { hotkeys.HandleMessage(id); });

    hotkeys.Register(1, MOD_WIN, 'L', []() { shell::power::PowerManager::Lock(); });
    hotkeys.Register(2, MOD_WIN, 'R', []() {
        ShellExecuteW(nullptr, L"open", L"explorer.exe", nullptr, nullptr, SW_SHOWNORMAL);
    });
    hotkeys.Register(3, MOD_WIN, VK_RETURN, [hTaskbar, &launcher]() {
        if (hTaskbar)
            launcher.ShowMenu(hTaskbar);
    });

    LOG_INFO("Shell host initialized.");

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    watcher.Stop();
    tray.Shutdown();
    GdiplusShutdown(gdiplusToken);
    CoUninitialize();
    LOG_INFO("Shell exiting.");
    return (int)msg.wParam;
}
