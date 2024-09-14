#pragma once
#include "../backend/allocation.hpp"
#include "DrawingContext.hpp"

namespace engine
{
    class GouraudMesh : public Object
    {
      public:
        Allocation     allocation;
        vk::DeviceSize vtx_offset;
        vk::DeviceSize idx_offset;
        uint32_t       count;

        GouraudMesh(Allocation allocation, vk::DeviceSize vtx_off, vk::DeviceSize idx_off, uint32_t count);

        void draw(DrawingContext &context) override;
        ~GouraudMesh();
    };
} // namespace engine