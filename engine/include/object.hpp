#pragma once
#include "transform.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <string>
#include "reflection/datastructure.hpp"

namespace engine
{
    /// Represents an object.
    ///
    /// Child objects must be explicitly stored.
    class Object
    {
      public:
        virtual ~Object();

        virtual void draw(struct DrawingContext &context, const glm::mat4 &parent_transform = {1.0}) = 0;

        virtual void process(double delta);
        virtual void physics_process(double delta);

        virtual const reflection::Datastructure *get_rep() const;

        Transform   transform;
        std::string name;

        std::weak_ptr<Object> parent;
    };

    extern const reflection::Datastructure OBJECT_REP;
} // namespace engine