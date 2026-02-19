cmake_minimum_required(VERSION 3.20)
project(western_rpg_proto VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

add_library(engine_core
    src/core/Application.cpp
    src/core/Timer.cpp
)

target_include_directories(engine_core PUBLIC src)
target_link_libraries(engine_core PUBLIC SDL2::SDL2)

add_library(engine_game
    src/game/Map.cpp
    src/game/Player.cpp
)

target_include_directories(engine_game PUBLIC src)

add_library(engine_render
    src/render/IsoMath.cpp
    src/render/Renderer.cpp
)

target_include_directories(engine_render PUBLIC src)
target_link_libraries(engine_render PUBLIC SDL2::SDL2 OpenGL::GL)

add_executable(game
    src/main.cpp
)

target_include_directories(game PRIVATE src)
target_link_libraries(game PRIVATE engine_core engine_game engine_render SDL2::SDL2 OpenGL::GL)

if(MSVC)
  target_compile_options(game PRIVATE /W4)
else()
  target_compile_options(game PRIVATE -Wall -Wextra -Wpedantic)
endif()
