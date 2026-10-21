add_subdirectory(cglm)

# XTest is optional; keep desktop window support without requiring libXtst.
# Enable it explicitly with -DSDL_X11_XTEST=ON when its headers are installed.
option(SDL_X11_XTEST "Enable XTest support" OFF)
add_subdirectory(SDL)

# Keep logger state and thread-local destructors outside reloadable game images.
set(SPDLOG_BUILD_SHARED ON CACHE BOOL "Share logging between engine and game modules" FORCE)
add_subdirectory(spdlog)
set(FLECS_SHARED ON CACHE BOOL "Build the shared ECS runtime" FORCE)
set(FLECS_STATIC OFF CACHE BOOL "Do not duplicate the ECS runtime in game modules" FORCE)
add_subdirectory(flecs)
