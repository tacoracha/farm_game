cmake_minimum_required(VERSION 3.20)
project(farm_game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
enable_testing()

add_library(farm_game_core STATIC
    src/common/Types.cpp
    src/core/Game.cpp
    src/inventory/PlayerState.cpp
    src/planting/PlantingSystem.cpp
    src/ranch/RanchSystem.cpp
    src/workshop/WorkshopSystem.cpp
    src/order/OrderSystem.cpp
    src/shop/ShopSystem.cpp
    src/persistence/SaveManager.cpp
)

target_include_directories(farm_game_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

if(MINGW)
    target_compile_options(farm_game_core PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
    target_compile_options(farm_game_core PRIVATE /W4)
endif()

add_executable(farm_game
    src/main.cpp
    src/ui/Win32FarmApp.cpp
)
target_link_libraries(farm_game PRIVATE farm_game_core)

if(WIN32)
    set_target_properties(farm_game PROPERTIES WIN32_EXECUTABLE TRUE)
    target_compile_definitions(farm_game PRIVATE UNICODE _UNICODE NOMINMAX)
    target_link_libraries(farm_game PRIVATE gdi32 user32 comdlg32)
endif()

add_executable(farm_game_tests
    tests/test_farm_game.cpp
)
target_link_libraries(farm_game_tests PRIVATE farm_game_core)
add_test(NAME farm_game_tests COMMAND farm_game_tests)

install(TARGETS farm_game RUNTIME DESTINATION .)
install(DIRECTORY assets DESTINATION .)
install(DIRECTORY licenses DESTINATION .)
install(FILES README.md DESTINATION . RENAME README.txt)

if(MINGW)
    get_filename_component(FARM_GAME_MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set(FARM_GAME_RUNTIME_DLLS)
    foreach(_dll libgcc_s_seh-1.dll libgcc_s_dw2-1.dll libstdc++-6.dll libwinpthread-1.dll)
        if(EXISTS "${FARM_GAME_MINGW_BIN_DIR}/${_dll}")
            list(APPEND FARM_GAME_RUNTIME_DLLS "${FARM_GAME_MINGW_BIN_DIR}/${_dll}")
        endif()
    endforeach()
    if(FARM_GAME_RUNTIME_DLLS)
        install(FILES ${FARM_GAME_RUNTIME_DLLS} DESTINATION .)
    endif()
endif()
