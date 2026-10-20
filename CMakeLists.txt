cmake_minimum_required(VERSION 4.0)

project(ior_engine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(thirdparty/cglm)

# XTest is optional; keep desktop window support without requiring libXtst.
# Enable it explicitly with -DSDL_X11_XTEST=ON when its headers are installed.
option(SDL_X11_XTEST "Enable XTest support" OFF)
add_subdirectory(thirdparty/SDL)

add_subdirectory(thirdparty/spdlog)
add_subdirectory(thirdparty/flecs)

set(SOURCES
    src/core.cpp
)

set(HEADERS
    src/core.h
)

add_executable(ior_engine ${SOURCES} ${HEADERS})

target_link_libraries(ior_engine PRIVATE cglm SDL3::SDL3 spdlog::spdlog flecs::flecs)