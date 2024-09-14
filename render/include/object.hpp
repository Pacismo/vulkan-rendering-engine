#pragma once
#include "transform.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

namespace engine
{
    /// Represents an object.
    ///
    /// Child objects must be explicitly stored.
    class Object
    {
      public:
        virtual ~Object() = default;

        virtual void draw(struct DrawingContext &) = 0;

        Transform transform;

        std::weak_ptr<Object> parent;
    };
} // namespace engine