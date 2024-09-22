#pragma once
#include "device_manager.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace engine
{
    class DescriptorPoolManager final
    {
      public:
        void init(std::shared_ptr<RenderDeviceManager> device_manager, uint32_t max_descriptors,
                  vk::DescriptorPoolCreateFlags flags = {});
        void destroy();

        static std::shared_ptr<DescriptorPoolManager> new_shared();
        static std::shared_ptr<DescriptorPoolManager> new_shared(std::shared_ptr<RenderDeviceManager> device_manager,
                                                                 uint32_t                             max_descriptors,
                                                                 vk::DescriptorPoolCreateFlags        flags = {});

        std::vector<vk::DescriptorSet> get(std::span<vk::DescriptorSetLayout> layouts);
        std::vector<vk::DescriptorSet> get(vk::DescriptorSetLayout layout, size_t count);
        vk::DescriptorSet              get(vk::DescriptorSetLayout layout);
        void                           reset();

        DescriptorPoolManager();
        DescriptorPoolManager(std::shared_ptr<RenderDeviceManager> device_manager, uint32_t max_descriptors,
                              vk::DescriptorPoolCreateFlags flags = {});
        ~DescriptorPoolManager();

        vk::DescriptorPool       get_pool();
        const vk::DescriptorPool get_pool() const;

      private:
        std::shared_ptr<RenderDeviceManager> m_device_manager;
        vk::Device                           m_device;
        vk::DescriptorPool                   m_pool;
    };
} // namespace engine
