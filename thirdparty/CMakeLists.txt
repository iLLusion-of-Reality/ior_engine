add_subdirectory(cglm)

# XTest is optional; keep desktop window support without requiring libXtst.
# Enable it explicitly with -DSDL_X11_XTEST=ON when its headers are installed.
option(SDL_X11_XTEST "Enable XTest support" OFF)
add_subdirectory(SDL)

add_subdirectory(spdlog)
add_subdirectory(flecs)
