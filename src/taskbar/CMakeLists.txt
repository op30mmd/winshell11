add_library(shell_taskbar STATIC
    TaskbarWindow.h TaskbarWindow.cpp
)

target_link_libraries(shell_taskbar PUBLIC shell_common)
