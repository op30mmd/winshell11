add_library(shell_desktop STATIC
    DesktopWindow.h DesktopWindow.cpp
    DesktopWidgets.h DesktopWidgets.cpp
    IconManager.h IconManager.cpp
    ContextMenuHelper.cpp
)

target_link_libraries(shell_desktop PUBLIC shell_common shell_ui gdiplus.lib)
