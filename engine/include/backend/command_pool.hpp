#pragma once
#include "device_manager.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace engine
{
    class CommandPoolManager final
    {
      public:
        void init(std::shared_ptr<RenderDeviceManager> device_manager, uint32_t queue_index);
        void destroy();

        static std::shared_ptr<CommandPoolManager> new_shared();
        static std::shared_ptr<CommandPoolManager> new_shared(std::shared_ptr<RenderDeviceManager> device_manager,
                                                              uint32_t                             max_descriptors);

        vk::CommandBuffer              get();
        std::vector<vk::CommandBuffer> get(uint32_t count);
        void                           reset();
        void                           free(vk::CommandBuffer buffer);

        CommandPoolManager();
        CommandPoolManager(std::shared_ptr<RenderDeviceManager> device_manager, uint32_t queue_index);
        ~CommandPoolManager();

        vk::CommandPool       get_pool();
        const vk::CommandPool get_pool() const;

      private:
        std::shared_ptr<RenderDeviceManager> m_device_manager;
        vk::Device                           m_device;
        vk::CommandPool                      m_pool;
    };
} // namespace engine
