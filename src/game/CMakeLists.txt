set(SOURCES
    game_library.cpp
)

set(HEADERS
    game_api.h
    game_library.h
)

add_library(ior_game STATIC ${SOURCES} ${HEADERS})
add_library(ior_engine::game ALIAS ior_game)

target_include_directories(ior_game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(ior_game PUBLIC ior_engine::ecs
    PRIVATE SDL3::SDL3 flecs::flecs)

export(
    TARGETS spdlog
    NAMESPACE spdlog::
    FILE "${PROJECT_BINARY_DIR}/sdk/spdlog-targets.cmake"
)

# Build-tree SDK: independent game projects use the exact same Flecs binary,
# headers and engine components instead of compiling another copy of Flecs.
file(GENERATE OUTPUT "${PROJECT_BINARY_DIR}/sdk/$<CONFIG>/ior_game_sdkConfig.cmake"
    CONTENT "
if(NOT TARGET ior_engine::flecs)
    add_library(ior_engine::flecs SHARED IMPORTED)
    set_target_properties(ior_engine::flecs PROPERTIES
        IMPORTED_LOCATION \"$<TARGET_FILE:flecs>\"
        IMPORTED_IMPLIB \"$<TARGET_LINKER_FILE:flecs>\"
        INTERFACE_INCLUDE_DIRECTORIES \"${PROJECT_SOURCE_DIR}/thirdparty/flecs/include\")
endif()
if(NOT TARGET ior_engine::game_sdk)
    add_library(ior_engine::game_sdk INTERFACE IMPORTED)
    set_target_properties(ior_engine::game_sdk PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES \"${CMAKE_CURRENT_SOURCE_DIR};${PROJECT_SOURCE_DIR}/src;${PROJECT_SOURCE_DIR}/thirdparty/cglm/include\"
        INTERFACE_LINK_LIBRARIES ior_engine::flecs
        INTERFACE_COMPILE_FEATURES cxx_std_20)
endif()

include(CMakeFindDependencyMacro)
find_dependency(Threads)
include(\"${PROJECT_BINARY_DIR}/sdk/spdlog-targets.cmake\")
")
