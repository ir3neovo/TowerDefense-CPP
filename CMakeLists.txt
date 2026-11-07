cmake_minimum_required(VERSION 3.28)
project(TowerDefense LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

# Internal code is linked into both executables; it is not a DLL API.
add_library(game_logic STATIC src/Game.cpp src/GameMap.cpp)
target_include_directories(game_logic PUBLIC include)
target_compile_features(game_logic PUBLIC cxx_std_17)
target_link_libraries(game_logic PUBLIC SFML::Graphics)

add_executable(main src/main.cpp)
target_link_libraries(main PRIVATE game_logic)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT main)

include(CTest)
if(BUILD_TESTING)
    add_executable(game_tests tests/GameTests.cpp)
    target_link_libraries(game_tests PRIVATE game_logic)
    add_test(NAME gameplay COMMAND game_tests)
endif()

if(WIN32 AND BUILD_SHARED_LIBS)
    # Each executable must also work when built on its own.
    foreach(executable IN ITEMS main game_tests)
        if(TARGET ${executable})
            add_custom_command(TARGET ${executable} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_RUNTIME_DLLS:${executable}> $<TARGET_FILE_DIR:${executable}>
                COMMAND_EXPAND_LISTS
                VERBATIM)
        endif()
    endforeach()
endif()
