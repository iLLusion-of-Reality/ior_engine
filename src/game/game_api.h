#pragma once

#include <flecs.h>
#include <ecs/components.h>

#include <cstdint>

// The boundary contains only fixed-size values, C pointers and Flecs C types.
// Bump this version when the SDK, component layouts or build ABI changes.
inline constexpr uint32_t ior_game_abi = 2;

struct ior_game_component_layout {
    uint32_t transform_size;
    uint32_t transform_alignment;
    uint32_t matrix_size;
    uint32_t matrix_alignment;
    bool operator==(const ior_game_component_layout&) const = default;
};

inline constexpr ior_game_component_layout ior_game_layout{
    sizeof(ior_engine::local_transform), alignof(ior_engine::local_transform),
    sizeof(ior_engine::local_to_world), alignof(ior_engine::local_to_world)};

struct ior_game_host {
    uint32_t size;
    ecs_world_t* world;
    ecs_entity_t root;
    ecs_entity_t local_transform;
    ecs_entity_t local_to_world;
};

struct ior_game_api {
    uint32_t size;
    uint32_t abi;
    uint32_t flecs_major;
    uint32_t flecs_minor;
    uint32_t flecs_patch;
    ior_game_component_layout components;
    decltype(&ecs_get_world) runtime;

    // Register everything below host.root. Copy host if needed after init.
    // Context belongs to this library; assign it before fallible initialization.
    // quit is called once per init attempt, including failure with null context.
    bool (*init)(const ior_game_host* host, void** context) noexcept;
    // Called once per progress, with Flecs delta time, on the world thread.
    void (*update)(void* context, float delta_time) noexcept;
    // Release external resources before the host deletes the game subtree.
    void (*quit)(void* context) noexcept;
};

using ior_game_query_fn = const ior_game_api* (*)(uint32_t abi, uint32_t host_size) noexcept;

#if defined(_WIN32)
#define IOR_GAME_EXPORT extern "C" __declspec(dllexport)
#else
#define IOR_GAME_EXPORT extern "C" __attribute__((visibility("default")))
#endif

// Define ior_game_query(uint32_t abi, uint32_t host_size) in the game with
// IOR_GAME_EXPORT and the ior_game_query_fn signature. Do not mark a declaration
// for export in the host. Query runs before removal and must have no side effects.
