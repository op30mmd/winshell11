add_library(shell_launcher STATIC
    AppLauncher.h AppLauncher.cpp
)

target_link_libraries(shell_launcher PUBLIC shell_common)
