#pragma once
#include "vertex.hpp"
#include <functional>
#include <memory>
#include <object.hpp>
#include <span>

namespace engine
{
    using DrawablePtr = std::shared_ptr<class Drawable>;

    /// Serves as a public interface to access functions pertaining to the rendering backend.
    class RenderBackend
    {
      public:
        using DrawHandler = std::function<void(struct DrawingContext &)>;

        virtual std::shared_ptr<Object> load(std::span<primitives::GouraudVertex> vertices,
                                             std::span<uint32_t>                  indices) = 0;

      protected:
        RenderBackend()          = default;
        virtual ~RenderBackend() = default;
    };
} // namespace engine