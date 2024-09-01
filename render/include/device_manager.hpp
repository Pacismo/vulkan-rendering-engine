#pragma once
#include <memory>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

namespace engine
{
    class VulkanDeviceManager final
    {
      public:
        VulkanDeviceManager(std::shared_ptr<class VulkanInstanceManager> instance_manager,
                            vk::PhysicalDevice                           physical_device);
        VulkanDeviceManager(const VulkanDeviceManager &) = delete;
        VulkanDeviceManager(VulkanDeviceManager &&)      = delete;
        ~VulkanDeviceManager();

      private:
        std::shared_ptr<spdlog::logger>              m_logger;
        std::shared_ptr<class VulkanInstanceManager> m_instance_manager = {};
        vk::DispatchLoaderDynamic                    m_dispatch         = {};
        vk::PhysicalDevice                           m_physical_device  = {};
        vk::Device                                   m_device           = {};
    };
} // namespace engine