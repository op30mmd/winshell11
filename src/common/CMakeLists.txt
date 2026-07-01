add_library(shell_common STATIC
    Config.h Config.cpp
    Logger.h Logger.cpp
)

target_include_directories(shell_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(shell_common PUBLIC Shell32.lib Ole32.lib)
