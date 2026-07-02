add_library(shell_backend SHARED
    exports.cpp
    ../power/PowerManager.cpp
    ../desktop/IconManager.cpp
    ../launcher/AppLauncher.cpp
)

target_link_libraries(shell_backend PRIVATE
    shell_common
    shell_watcher
    PowrProf.lib
    Wlanapi.lib
    Iphlpapi.lib
    Winmm.lib
    ole32.lib
    oleaut32.lib
    wbemuuid.lib
    Shlwapi.lib
)

target_include_directories(shell_backend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
set_target_properties(shell_backend PROPERTIES PREFIX "")
