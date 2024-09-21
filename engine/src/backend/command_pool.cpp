#include "backend/command_pool.hpp"

namespace engine
{
    void CommandPoolManager::init(std::shared_ptr<RenderDeviceManager> device_manager, uint32_t queue_index)
    {
        m_device         = device_manager->device;
        m_device_manager = device_manager;

        vk::CommandPoolCreateInfo create_info = {
            .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = queue_index,
        };

        m_pool = m_device.createCommandPool(create_info);
    }

    void CommandPoolManager::destroy()
    {
        if (m_pool)
            m_device.destroyCommandPool(m_pool);

        m_pool           = nullptr;
        m_device         = nullptr;
        m_device_manager = nullptr;
    }

    vk::CommandBuffer CommandPoolManager::get()
    {
        return get(1)[0];
    }

    std::vector<vk::CommandBuffer> CommandPoolManager::get(uint32_t count)
    {
        vk::CommandBufferAllocateInfo allocate_info = {
            .commandPool        = m_pool,
            .level              = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = count,
        };

        return m_device.allocateCommandBuffers(allocate_info);
    }

    void CommandPoolManager::reset()
    {
        m_device.resetCommandPool(m_pool);
    }

    void CommandPoolManager::free(vk::CommandBuffer buffer)
    {
        m_device.freeCommandBuffers(m_pool, buffer);
    }

    CommandPoolManager::CommandPoolManager()
        : m_device_manager(nullptr)
        , m_device(nullptr)
        , m_pool(nullptr)
    { }

    CommandPoolManager::CommandPoolManager(std::shared_ptr<RenderDeviceManager> device_manager, uint32_t queue_index)
    {
        init(device_manager, queue_index);
    }

    CommandPoolManager::~CommandPoolManager()
    {
        destroy();
    }

    vk::CommandPool CommandPoolManager::get_pool()
    {
        return m_pool;
    }

    const vk::CommandPool CommandPoolManager::get_pool() const
    {
        return m_pool;
    }
} // namespace engine
