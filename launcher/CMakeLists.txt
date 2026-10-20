cmake_minimum_required(VERSION 3.20)
project(GoW2Launcher LANGUAGES C CXX)

# Launcher for the God of War II native port. Same libraries and versions as PS2Recomp's runtime
# (PS2Recomp/ps2xRuntime/CMakeLists.txt): raylib 5.5, Dear ImGui v1.92.7-docking and rlImGui Raylib_5_5.
# scripts/build-launcher.ps1 points FETCHCONTENT_SOURCE_DIR_* at the copies the runner build already
# downloaded (out/build/_deps), so nothing is fetched twice.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
# raylib's built-in F12 screenshot is off, as in the runtime
set(CUSTOMIZE_BUILD ON CACHE BOOL "" FORCE)
set(SUPPORT_SCREEN_CAPTURE OFF CACHE BOOL "" FORCE)
set(SUPPORT_GIF_RECORDING OFF CACHE BOOL "" FORCE)

FetchContent_Declare(raylib
    GIT_REPOSITORY "https://github.com/raysan5/raylib.git"
    GIT_TAG "5.5"
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(raylib)

FetchContent_Declare(imgui
    GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
    GIT_TAG "v1.92.7-docking"
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(imgui) # no CMakeLists.txt upstream: this only downloads it
add_library(launcher_imgui STATIC
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp")
target_include_directories(launcher_imgui PUBLIC "${imgui_SOURCE_DIR}")

FetchContent_Declare(rlImGui
    GIT_REPOSITORY "https://github.com/raylib-extras/rlImGui.git"
    GIT_TAG "Raylib_5_5"
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(rlImGui) # no CMakeLists.txt upstream: this only downloads it
add_library(launcher_rlimgui STATIC "${rlimgui_SOURCE_DIR}/rlImGui.cpp")
target_include_directories(launcher_rlimgui PUBLIC "${rlimgui_SOURCE_DIR}")
target_link_libraries(launcher_rlimgui PUBLIC raylib launcher_imgui)

add_executable(GoW2Launcher src/main.cpp)
target_link_libraries(GoW2Launcher PRIVATE raylib launcher_imgui launcher_rlimgui)
if(MSVC)
    # a window application: no console behind the launcher (raylib keeps its plain main())
    set_target_properties(GoW2Launcher PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_options(GoW2Launcher PRIVATE "/ENTRY:mainCRTStartup")
    target_compile_options(GoW2Launcher PRIVATE /utf-8)
endif()

# the fonts travel next to the executable
add_custom_command(TARGET GoW2Launcher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/fonts" "$<TARGET_FILE_DIR:GoW2Launcher>/fonts")
