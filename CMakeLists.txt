cmake_minimum_required(VERSION 3.20)
project(western_rpg_proto VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FETCH_SDL2_IF_MISSING "Download SDL2 at configure time if no system package is found" OFF)

find_package(SDL2 CONFIG QUIET)

if(NOT TARGET SDL2::SDL2)
  find_package(SDL2 QUIET)
endif()

if(NOT TARGET SDL2::SDL2)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(PC_SDL2 QUIET sdl2)
  endif()

  if(PC_SDL2_FOUND)
    add_library(SDL2::SDL2 INTERFACE IMPORTED)
    target_include_directories(SDL2::SDL2 INTERFACE ${PC_SDL2_INCLUDE_DIRS})
    target_compile_options(SDL2::SDL2 INTERFACE ${PC_SDL2_CFLAGS_OTHER})
    target_link_libraries(SDL2::SDL2 INTERFACE ${PC_SDL2_LINK_LIBRARIES})
    target_link_options(SDL2::SDL2 INTERFACE ${PC_SDL2_LDFLAGS_OTHER})
  endif()
endif()

if(NOT TARGET SDL2::SDL2)
  if(FETCH_SDL2_IF_MISSING)
    include(FetchContent)
    message(STATUS "SDL2 was not found locally; fetching SDL2 from upstream.")
    FetchContent_Declare(
      SDL2
      GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
      GIT_TAG release-2.30.11
      GIT_SHALLOW TRUE
    )
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(SDL2)
  endif()
endif()

if(NOT TARGET SDL2::SDL2)
  message(FATAL_ERROR "SDL2 was not found. Install the SDL2 development package, set SDL2_DIR/CMAKE_PREFIX_PATH, or enable FETCH_SDL2_IF_MISSING=ON (requires network access).")
endif()

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
