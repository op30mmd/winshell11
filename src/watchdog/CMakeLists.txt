add_executable(shellwatchdog
    main.cpp
)

target_link_libraries(shellwatchdog PRIVATE shell_common)
