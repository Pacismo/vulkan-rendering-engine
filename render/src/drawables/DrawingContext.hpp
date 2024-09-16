#pragma once
#include "../backend/vulkan_backend.hpp"
#include "object.hpp"
#include "vertex.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <render_backend.hpp>
#include <span>
#include <vulkan/vulkan.hpp>

namespace engine
{
    struct DrawingContext
    {
        VulkanBackend           *backend;
        DescriptorPoolManager   *descriptor_pool;
        size_t                   frame_index;
        uint32_t                 swapchain_image_index;
        vk::DescriptorBufferInfo vp_buffer_info;
        vk::CommandBuffer        cmd;
    };
} // namespace engine