#pragma once
#include <glm/glm.hpp>

namespace engine::primitives
{
    using glm::vec2;
    using glm::vec3;

    struct GouraudVertex
    {
        vec3 position = {};
        vec3 color    = {};
    };

    struct TexturedVertex
    {
        vec3 position = {};
        vec2 uv       = {};
    };
} // namespace engine::primitives
