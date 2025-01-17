#include "backend/allocator.hpp"
#include "backend/instance_manager.hpp"

using std::shared_ptr, std::make_shared;

namespace engine
{
    shared_ptr<VulkanAllocator> VulkanAllocator::new_shared(RenderDeviceManager::Shared device_manager)
    {
        if (shared_ptr<VulkanAllocator> ptr = device_manager->allocator.lock())
            return ptr;

        auto ptr                  = shared_ptr<VulkanAllocator>(new VulkanAllocator(device_manager));
        device_manager->allocator = ptr;
        return ptr;
    }

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

    RenderDeviceManager::Shared VulkanAllocator::get_device_manager()
    {
        return m_device_manager;
    }

    VulkanAllocator::~VulkanAllocator()
    {
        vmaDestroyAllocator(m_allocator);
    }
} // namespace engine