#pragma once
#include "vertex.hpp"
#include <memory>
#include <render_backend.hpp>
#include <span>

namespace engine
{
    // TODO: implement interface for engine objects to be drawn by the user
    class Drawable
    {
      public:
        /// Public interface to draw to the screen.
        ///
        /// Not designed for end-user implementation.
        virtual void draw(class DrawingContext &context) = 0;
        virtual ~Drawable()                              = default;
    };
} // namespace engine