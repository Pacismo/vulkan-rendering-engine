#pragma once
#include "backend/allocation.hpp"
#include "constants.hpp"
#include "object.hpp"


namespace engine
{
    class GouraudMesh : public Object
    {
      public:
        Allocation                                           allocation;
        TypedHostVisibleAllocation<glm::mat4[MAX_IN_FLIGHT]> model_matrix;
        vk::DeviceSize                                       vtx_offset;
        vk::DeviceSize                                       idx_offset;
        uint32_t                                             count;

        GouraudMesh(Allocation allocation, vk::DeviceSize vtx_off, vk::DeviceSize idx_off, uint32_t count);

        void draw(struct DrawingContext &context, const glm::mat4 &parent_transform = {1.0}) override;
        ~GouraudMesh();
    };
} // namespace engine
