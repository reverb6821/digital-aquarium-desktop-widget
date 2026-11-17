# Desktop simulator for the digital aquarium (Linux, SDL2).
#   sudo dnf install SDL2-devel cmake ninja-build gcc
#   cmake -S sim -B build-sim -G Ninja && cmake --build build-sim && ./build-sim/aquarium_sim
cmake_minimum_required(VERSION 3.16)
project(aquarium_sim C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
add_compile_options(-Wall -Wextra)
add_compile_definitions(_DEFAULT_SOURCE)

set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/../aquarium/src)
file(GLOB PORTABLE_SOURCES
  ${SRC}/engine/*.c ${SRC}/scene/*.c ${SRC}/config/*.c ${SRC}/ui/*.c ${SRC}/app/*.c ${SRC}/i18n/*.c)

add_library(aquarium_core STATIC ${PORTABLE_SOURCES} hal_sim.c png_write.c)
target_link_libraries(aquarium_core PUBLIC m)

# Headless end-to-end test (no SDL needed): ./build-sim/aquarium_headless out/
add_executable(aquarium_headless headless.c)
target_link_libraries(aquarium_headless aquarium_core)

find_package(SDL2 QUIET)
if(SDL2_FOUND)
  add_executable(aquarium_sim main_sdl.c)
  if(TARGET SDL2::SDL2)
    target_link_libraries(aquarium_sim aquarium_core SDL2::SDL2)
  else()
    target_include_directories(aquarium_sim PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(aquarium_sim aquarium_core ${SDL2_LIBRARIES})
  endif()
else()
  message(WARNING "SDL2 not found: only aquarium_headless will be built (sudo dnf install SDL2-devel)")
endif()
