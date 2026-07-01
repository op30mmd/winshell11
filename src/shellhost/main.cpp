#include <windows.h>
#include <shlobj.h>
#include "common/Config.h"
#include "common/Logger.h"
#include "MainWindow.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    shell::common::Logger::Initialize("shellhost");
    LOG_INFO("Shell starting...");

    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))) {
        LOG_ERROR("Failed to initialize COM");
        return 1;
    }

    auto config = shell::common::Config::Load();

    shell::host::MainWindow mainWindow;
    if (!mainWindow.Create(hInstance)) {
        LOG_ERROR("Failed to create main window");
        CoUninitialize();
        return 1;
    }

    LOG_INFO("Shell host initialized.");

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CoUninitialize();
    LOG_INFO("Shell exiting.");
    return (int)msg.wParam;
}
