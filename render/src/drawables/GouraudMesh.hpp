#pragma once
#include "../backend/allocation.hpp"
#include "DrawingContext.hpp"
#include <array>

namespace engine
{
    class GouraudMesh : public Object
    {
      public:
        Allocation                                                      allocation;
        TypedHostVisibleAllocation<glm::mat4[VulkanBackend::IN_FLIGHT]> model_matrix;
        vk::DeviceSize                                                  vtx_offset;
        vk::DeviceSize                                                  idx_offset;
        uint32_t                                                        count;
        vk::DescriptorPool                                              descriptor_pool;
        std::array<vk::DescriptorSet, VulkanBackend::IN_FLIGHT>         descriptor_sets;

        GouraudMesh(Allocation allocation, vk::DeviceSize vtx_off, vk::DeviceSize idx_off, uint32_t count);

        void draw(DrawingContext &context, const glm::mat4 &parent_transform = {1.0}) override;
        ~GouraudMesh();
    };
} // namespace engine
