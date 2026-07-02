cmake_minimum_required(VERSION 3.20)
project(CustomShell LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /WX /analyze)
endif()

add_subdirectory(src/common)
add_subdirectory(src/windowwatcher)
add_subdirectory(src/backend)
