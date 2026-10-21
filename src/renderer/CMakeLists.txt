set(SOURCES
    renderer.cpp
)

set(HEADERS
    renderer.h
)

add_library(ior_renderer STATIC ${SOURCES} ${HEADERS})
add_library(ior_engine::renderer ALIAS ior_renderer)

target_include_directories(ior_renderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ior_renderer PUBLIC cglm PRIVATE SDL3::SDL3 flecs::flecs spdlog::spdlog)
