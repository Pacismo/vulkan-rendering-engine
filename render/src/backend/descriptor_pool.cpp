#include "descriptor_pool.hpp"
#include "vulkan_backend.hpp"

using std::shared_ptr, std::span, std::vector;

namespace engine
{
    void DescriptorPoolManager::init(std::shared_ptr<RenderDeviceManager> device_manager)
    {
        m_device_manager = device_manager;
        m_device         = device_manager->device;

        vk::DescriptorPoolSize size = {
            .descriptorCount = VulkanBackend::IN_FLIGHT,
        };

        vk::DescriptorPoolCreateInfo dp_ci = {
            .maxSets       = VulkanBackend::MAX_DESCRIPTORS,
            .poolSizeCount = 1,
            .pPoolSizes    = &size,
        };

        m_pool = m_device.createDescriptorPool(dp_ci);
    }

    void DescriptorPoolManager::destroy()
    {
        if (m_pool)
            m_device.destroyDescriptorPool(m_pool);

        m_device_manager = nullptr;
        m_pool           = nullptr;
    }

    vector<vk::DescriptorSet> DescriptorPoolManager::get(span<vk::DescriptorSetLayout> layouts)
    {
        return m_device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo {
            .descriptorPool     = m_pool,
            .descriptorSetCount = (uint32_t)layouts.size(),
            .pSetLayouts        = layouts.data(),
        });
    }

    vk::DescriptorSet DescriptorPoolManager::get(vk::DescriptorSetLayout layout)
    {
        return get(span(&layout, 1))[0];
    }

    void DescriptorPoolManager::reset()
    {
        m_device.resetDescriptorPool(m_pool);
    }

    DescriptorPoolManager::DescriptorPoolManager()
        : m_device_manager(nullptr)
        , m_device(nullptr)
        , m_pool(nullptr)
    { }

    DescriptorPoolManager::DescriptorPoolManager(shared_ptr<RenderDeviceManager> device_manager)
    {
        init(device_manager);
    }

    DescriptorPoolManager::~DescriptorPoolManager()
    {
        destroy();
    }
} // namespace engine