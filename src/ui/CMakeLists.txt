add_library(shell_ui STATIC
    Theme.h
    Renderer.h Renderer.cpp
    Animation.h Animation.cpp
    Widget.h Widget.cpp
    Controls.h Controls.cpp
    UiHost.h UiHost.cpp
)

target_include_directories(shell_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(shell_ui PUBLIC shell_common)
