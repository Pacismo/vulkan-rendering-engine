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
        /// Creates a new shared allocator if one has not been made for the device.
        ///
        /// If the device manager already has a shared allocator, returns that pointer.
        static std::shared_ptr<VulkanAllocator> new_shared(RenderDeviceManager::Shared device_manager);

        operator VmaAllocator();
        operator const VmaAllocator() const;

        RenderDeviceManager::Shared get_device_manager();

        ~VulkanAllocator();

      private:
        VulkanAllocator(RenderDeviceManager::Shared device_manager);

        RenderDeviceManager::Shared m_device_manager = {};
        VmaAllocator                m_allocator      = {};
    };
} // namespace engine