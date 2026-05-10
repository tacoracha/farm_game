cmake_minimum_required(VERSION 3.16)
project(farm_game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# CMake 3.24+: silence FetchContent extract timestamp warnings for URL archives.
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)

# Prefer HTTPS ZIP downloads over GIT_REPOSITORY so Visual Studio / CI works without Git on PATH
# and avoids ExternalProject git-clone failures behind corporate proxies.

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    glfw
    URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
)
FetchContent_MakeAvailable(glfw)

# Dear ImGui has no root CMakeLists meant for add_subdirectory; only populate sources.
FetchContent_Declare(
    imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.91.8.zip
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

set(IMGUI_DIR ${imgui_SOURCE_DIR})

add_library(
    imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw)

add_library(
    farm_game_lib STATIC
    src/ItemCatalog.cpp
    src/ChickenCoop.cpp
    src/FeedMill.cpp
    src/Warehouse.cpp
    src/PlayerState.cpp
    src/PlantingSystem.cpp
    src/ShopSystem.cpp
    src/RanchSystem.cpp
    src/WorkshopSystem.cpp
    src/OrderSystem.cpp
    src/Game.cpp
)
target_include_directories(farm_game_lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_executable(
    farm_game
    src/main.cpp
    src/ui/AppUi.cpp
    src/ui/UiStyle.cpp
    src/ui/UiWidgets.cpp
    src/ui/panels/TopStatusBar.cpp
    src/ui/panels/FarmPanel.cpp
    src/ui/panels/WorkshopPanel.cpp
    src/ui/panels/CoopPanel.cpp
    src/ui/panels/SidebarPanel.cpp
)
target_link_libraries(farm_game PRIVATE farm_game_lib imgui)
if(WIN32)
    target_link_libraries(farm_game PRIVATE opengl32)
endif()

add_executable(farm_game_demo src/demo_main.cpp src/DemoRunner.cpp)
target_link_libraries(farm_game_demo PRIVATE farm_game_lib)

add_executable(farm_game_tests tests/test_farm_game.cpp)
target_link_libraries(farm_game_tests PRIVATE farm_game_lib)
