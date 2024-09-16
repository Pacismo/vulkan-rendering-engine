#include "allocator.hpp"
#include "instance_manager.hpp"

namespace engine
{
    VulkanAllocator::VulkanAllocator(RenderDeviceManager::Shared device_manager)
        : m_device_manager(device_manager)
    {
        VmaAllocatorCreateInfo alloc_ci = {
            .flags            = 0,
            .physicalDevice   = m_device_manager->physical_device,
            .device           = m_device_manager->device,
            .pVulkanFunctions = nullptr,
            .instance         = m_device_manager->instance_manager->instance,
            .vulkanApiVersion = vk::ApiVersion13,
        };

        if (VkResult result = vmaCreateAllocator(&alloc_ci, &m_allocator))
            throw VulkanException(result, "Failed to create device allocator");
    }

    VulkanAllocator::operator VmaAllocator()
    {
        return m_allocator;
    }

    VulkanAllocator::operator const VmaAllocator() const
    {
        return m_allocator;
    }

    VulkanAllocator::~VulkanAllocator()
    {
        vmaDestroyAllocator(m_allocator);
    }
} // namespace engine