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
add_subdirectory(src/WinUIShell)

# Installer target (Inno Setup)
find_program(ISCC_EXE ISCC)
if(ISCC_EXE)
    add_custom_target(installer
        COMMAND ${ISCC_EXE} ${CMAKE_SOURCE_DIR}/installer/CustomShell.iss
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Building Setup Installer"
    )
    add_dependencies(installer WinUIShell)
endif()
