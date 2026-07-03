find_program(DOTNET_EXE dotnet REQUIRED)
find_program(MSBUILD_EXE msbuild)

# On Windows CI runners, MSBuild is usually available after setup-msbuild or in the path.
# If not found, we'll try to find it or fallback to dotnet build (less ideal for WinUI 3).
if(NOT MSBUILD_EXE)
    set(BUILD_TOOL ${DOTNET_EXE})
    set(BUILD_COMMAND build)
else()
    set(BUILD_TOOL ${MSBUILD_EXE})
    set(BUILD_COMMAND "")
endif()

# We need to pass the configuration correctly.
# msbuild uses /p:Configuration=Release
# dotnet build uses --configuration Release

add_custom_target(WinUIShell
    COMMAND ${DOTNET_EXE} restore ${CMAKE_CURRENT_SOURCE_DIR}/WinUIShell.csproj
    COMMAND ${BUILD_TOOL} ${BUILD_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/WinUIShell.csproj
            /p:Configuration=$<CONFIG>
            /p:Platform=x64
            /p:UseXamlCompilerExecutable=false
            /p:RestorePackages=false
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building WinUI 3 Frontend"
)

# Ensure the backend DLL is built before the frontend
add_dependencies(WinUIShell shell_backend)
