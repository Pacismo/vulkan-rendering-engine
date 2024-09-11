#pragma once
#include "vertex.hpp"
#include <memory>
#include <span>

namespace engine
{
    /// Serves as a public interface to access functions pertaining to the rendering backend.
    class RenderBackend
    {
      protected:
        class MeshHandle
        {
          public:
            virtual ~MeshHandle() = default;

            virtual void visible(bool value) = 0;
            virtual bool visible() const     = 0;
        };

      public:
        using MeshHandlePtr = std::shared_ptr<RenderBackend::MeshHandle>;

        virtual MeshHandlePtr load(std::span<primitives::GouraudVertex> vertices, std::span<uint32_t> indices) = 0;

      protected:
        RenderBackend()          = default;
        virtual ~RenderBackend() = default;
    };
} // namespace engine