#pragma once
#include <glm/glm.hpp>

namespace engine
{
    constexpr size_t MAX_IN_FLIGHT   = 2;
    constexpr size_t MAX_DESCRIPTORS = 128;
    constexpr float  DEFAULT_FOV     = 70.0;

    constexpr glm::vec3 X_AXIS = {1.0, 0.0, 0.0};
    constexpr glm::vec3 Y_AXIS = {0.0, 1.0, 0.0};
    constexpr glm::vec3 Z_AXIS = {0.0, 0.0, 1.0};
} // namespace engine