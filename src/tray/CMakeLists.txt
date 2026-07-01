add_library(shell_tray STATIC
    TrayHost.h TrayHost.cpp
)

target_link_libraries(shell_tray PUBLIC shell_common)
