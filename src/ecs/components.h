#pragma once
#include <cglm/cglm.h>

namespace ior_engine {
struct local_transform {
    vec3 position = GLM_VEC3_ZERO_INIT;
    versor rotation = GLM_QUAT_IDENTITY_INIT;
    vec3 scale = GLM_VEC3_ONE_INIT;
};

struct local_to_world {
    mat4 transform = GLM_MAT4_IDENTITY_INIT;
};

} // namespace ior_engine