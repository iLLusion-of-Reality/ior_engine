add_subdirectory(ecs)
add_subdirectory(game)
add_subdirectory(core)

add_executable(ior_engine)
target_link_libraries(ior_engine PRIVATE ior_engine::core)

set_target_properties(ior_engine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
