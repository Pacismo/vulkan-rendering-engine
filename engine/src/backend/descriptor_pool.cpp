#include "backend/descriptor_pool.hpp"
#include <memory>
#include <vulkan/vulkan_handles.hpp>

using std::shared_ptr, std::span, std::vector;

namespace engine
{
    void DescriptorPoolManager::init(std::shared_ptr<RenderDeviceManager> device_manager, uint32_t max_descriptors)
    {
        m_device_manager = device_manager;
        m_device         = device_manager->device;

        vk::DescriptorPoolSize size = {
            .type            = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = max_descriptors,
        };

        vk::DescriptorPoolCreateInfo dp_ci = {
            .maxSets       = max_descriptors,
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
        m_device         = nullptr;
        m_pool           = nullptr;
    }

    std::shared_ptr<DescriptorPoolManager> DescriptorPoolManager::new_shared()
    {
        return std::shared_ptr<DescriptorPoolManager>(new DescriptorPoolManager());
    }

    std::shared_ptr<DescriptorPoolManager> DescriptorPoolManager::new_shared(
        std::shared_ptr<RenderDeviceManager> device_manager, uint32_t max_descriptors)
    {
        return std::shared_ptr<DescriptorPoolManager>(new DescriptorPoolManager(device_manager, max_descriptors));
    }

    vector<vk::DescriptorSet> DescriptorPoolManager::get(span<vk::DescriptorSetLayout> layouts)
    {
        return m_device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo {
            .descriptorPool     = m_pool,
            .descriptorSetCount = (uint32_t)layouts.size(),
            .pSetLayouts        = layouts.data(),
        });
    }

    std::vector<vk::DescriptorSet> DescriptorPoolManager::get(vk::DescriptorSetLayout layout, size_t count)
    {
        std::vector<vk::DescriptorSet> sets(count);

        for (auto &set : sets)
            set = get(layout);

        return sets;
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

    DescriptorPoolManager::DescriptorPoolManager(shared_ptr<RenderDeviceManager> device_manager,
                                                 uint32_t                        max_descriptors)
    {
        init(device_manager, max_descriptors);
    }

    DescriptorPoolManager::~DescriptorPoolManager()
    {
        destroy();
    }

    vk::DescriptorPool DescriptorPoolManager::get_pool()
    {
        return m_pool;
    }

    const vk::DescriptorPool DescriptorPoolManager::get_pool() const
    {
        return m_pool;
    }
} // namespace engine
