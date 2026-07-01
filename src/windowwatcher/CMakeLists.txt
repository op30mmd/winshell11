add_library(shell_watcher STATIC
    WindowWatcher.h WindowWatcher.cpp
)

target_link_libraries(shell_watcher PUBLIC shell_common)
