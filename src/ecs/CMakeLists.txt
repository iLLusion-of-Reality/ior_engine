add_library(ior_ecs STATIC ecs.cpp ecs.h)
add_library(ior_engine::ecs ALIAS ior_ecs)

target_include_directories(ior_ecs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ior_ecs PRIVATE flecs::flecs)
