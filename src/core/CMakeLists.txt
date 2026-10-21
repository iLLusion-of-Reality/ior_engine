set(SOURCES
    core.cpp
)

set(HEADERS
    core.h
)

# Keep SDL's entry point and callbacks in core.cpp. Object linkage ensures
# that the executable includes them without requiring a separate main file.
add_library(ior_core OBJECT ${SOURCES} ${HEADERS})
add_library(ior_engine::core ALIAS ior_core)

target_include_directories(ior_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ior_core PUBLIC SDL3::SDL3 ior_engine::ecs ior_engine::renderer ior_engine::game
    PRIVATE flecs::flecs cglm spdlog::spdlog)
