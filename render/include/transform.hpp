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
            glm::mat4 mat = {1.0};

            mat = glm::translate(mat, location);
            mat = glm::rotate(mat, rotation.x, Y_AXIS);
            mat = glm::rotate(mat, rotation.y, X_AXIS);
            mat = glm::rotate(mat, rotation.z, Z_AXIS);
            mat = glm::scale(mat, scale);

            return mat;
        }

        inline operator glm::mat4() const { return get_transform_matrix(); }
    };
} // namespace engine