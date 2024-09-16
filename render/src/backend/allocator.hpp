#pragma once
#include "device_manager.hpp"
#include <memory>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace engine
{
    class VulkanAllocator
    {
      public:
        VulkanAllocator() = default;
        VulkanAllocator(RenderDeviceManager::Shared device_manager);

        operator VmaAllocator();
        operator const VmaAllocator() const;

        ~VulkanAllocator();

      private:
        RenderDeviceManager::Shared m_device_manager = {};
        VmaAllocator                m_allocator      = {};
    };
} // namespace engine