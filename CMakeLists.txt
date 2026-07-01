cmake_minimum_required(VERSION 3.20)
project(CustomShell LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force MSVC flags if on Windows
if(MSVC)
    add_compile_options(/W4 /WX /analyze)
endif()

add_subdirectory(src/common)
add_subdirectory(src/desktop)
add_subdirectory(src/windowwatcher)
add_subdirectory(src/taskbar)
add_subdirectory(src/tray)
add_subdirectory(src/flyout)
add_subdirectory(src/launcher)
add_subdirectory(src/shellhost)
add_subdirectory(src/watchdog)
