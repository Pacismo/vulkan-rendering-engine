#pragma once
#include "constants.hpp"
#include <glm/glm.hpp>
#include <span>
#include <vulkan/vulkan.hpp>

namespace engine
{
    struct DrawingContext
    {
        class VulkanBackend                          *backend;
        std::span<vk::DescriptorSet, MAX_DESCRIPTORS> descriptors;
        size_t                                        used_descriptors;
        size_t                                        frame_index;
        uint32_t                                      swapchain_image_index;
        vk::DescriptorBufferInfo                      vp_buffer_info;
        vk::CommandBuffer                             cmd;
    };
} // namespace engine
