#pragma once
#include "backend/allocation.hpp"
#include "constants.hpp"
#include "drawing_context.hpp"
#include <array>

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
        vk::DescriptorPool                                   descriptor_pool;
        std::array<vk::DescriptorSet, MAX_IN_FLIGHT>         descriptor_sets;

        GouraudMesh(Allocation allocation, vk::DeviceSize vtx_off, vk::DeviceSize idx_off, uint32_t count);

        void draw(DrawingContext &context, const glm::mat4 &parent_transform = {1.0}) override;
        ~GouraudMesh();
    };
} // namespace engine
