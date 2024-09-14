#pragma once
#include "constants.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace engine
{
    struct Transform
    {
        glm::vec3 location = {0.0, 0.0, 0.0};
        glm::vec3 rotation = {0.0, 0.0, 0.0};
        glm::vec3 scale    = {1.0, 1.0, 1.0};

        inline glm::mat4 get_transform_matrix() const
        {
            glm::mat4 mat = {};

            glm::translate(mat, location);
            glm::rotate(mat, rotation.y, Y_AXIS);
            glm::rotate(mat, rotation.y, X_AXIS);
            glm::rotate(mat, rotation.y, Z_AXIS);
            glm::scale(mat, scale);

            return mat;
        }
    };
} // namespace engine