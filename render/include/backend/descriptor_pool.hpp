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
        void init(std::shared_ptr<RenderDeviceManager> device_manager);
        void destroy();

        std::vector<vk::DescriptorSet> get(std::span<vk::DescriptorSetLayout> layouts);
        vk::DescriptorSet              get(vk::DescriptorSetLayout layout);
        void                           reset();

        DescriptorPoolManager();
        DescriptorPoolManager(std::shared_ptr<RenderDeviceManager> device_manager);
        ~DescriptorPoolManager();

      private:
        std::shared_ptr<RenderDeviceManager> m_device_manager;
        vk::Device                           m_device;
        vk::DescriptorPool                   m_pool;
    };
} // namespace engine