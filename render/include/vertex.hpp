#pragma once
#include <glm/glm.hpp>

namespace engine::primitives
{
    using glm::vec2;
    using glm::vec3;

    struct Vertex
    {
        vec2 position = {};
        vec3 color    = {};
    };
} // namespace engine::primitives
