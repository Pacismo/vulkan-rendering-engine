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

        virtual void draw(struct DrawingContext &context, const glm::mat4 &parent_transform = {1.0}) = 0;

        inline virtual void process(double delta) { }
        inline virtual void physics_process(double delta) { }

        Transform transform;

        std::weak_ptr<Object> parent;
    };
} // namespace engine