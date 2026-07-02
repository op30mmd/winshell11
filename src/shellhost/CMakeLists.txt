add_executable(shellhost
    main.cpp
    MainWindow.h MainWindow.cpp
    HotkeyManager.h HotkeyManager.cpp
    ../power/PowerManager.h ../power/PowerManager.cpp
    ../flyout/FlyoutWindow.h ../flyout/FlyoutWindow.cpp
)

target_link_libraries(shellhost PRIVATE
    shell_common
    shell_ui
    shell_desktop
    shell_watcher
    shell_taskbar
    shell_tray
    shell_launcher
    PowrProf.lib
    Wlanapi.lib
    Iphlpapi.lib
    Winmm.lib
    ole32.lib
    oleaut32.lib
    wbemuuid.lib
)
set_target_properties(shellhost PROPERTIES WIN32_EXECUTABLE TRUE)
