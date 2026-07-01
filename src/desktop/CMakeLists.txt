add_library(shell_desktop STATIC
    DesktopWindow.h DesktopWindow.cpp
    IconManager.h IconManager.cpp
    ContextMenuHelper.cpp
)

target_link_libraries(shell_desktop PUBLIC shell_common gdiplus.lib)
