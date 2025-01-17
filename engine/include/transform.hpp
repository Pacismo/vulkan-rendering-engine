#pragma once
#include "constants.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

inline constexpr double operator"" _deg(long double value)
{
    return glm::radians(value);
}

inline constexpr double operator"" _rad(long double value)
{
    return value;
}

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

    struct CameraTransform
    {
        glm::vec3 location = {0.0, 0.0, 0.0};
        glm::vec2 rotation = {0.0, 0.0};

        static constexpr glm::vec3 up      = Z_AXIS;
        static constexpr glm::vec3 forward = Y_AXIS;
        static constexpr glm::vec3 right   = X_AXIS;

        inline glm::vec3 get_forward_vector() const
        {
            return glm::normalize(glm::vec3(get_facing_matrix() * glm::vec4(forward, 1.0)));
        }

        inline glm::mat4 get_facing_matrix() const
        {
            return glm::rotate(glm::rotate(glm::mat4 {1.0}, rotation.x, up), rotation.y, right);
        }

        inline glm::mat4 get_transformation_matrix() const
        {
            return glm::lookAt(location, location + get_forward_vector(), up);
        }

        inline operator glm::mat4() const { return get_transformation_matrix(); }
    };
} // namespace engine