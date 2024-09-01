#pragma once

#include "exceptions.hpp"
#include "version.hpp"

#include <memory>
#include <spdlog/spdlog.h>
#include <string_view>
#include <vulkan/vulkan.hpp>

namespace engine
{
    class VulkanInstanceManager final
    {
        friend class VulkanDeviceManager;

      public:
        VulkanInstanceManager(std::string_view app_name, Version app_version);
        VulkanInstanceManager(VulkanInstanceManager &&)      = delete;
        VulkanInstanceManager(const VulkanInstanceManager &) = delete;
        ~VulkanInstanceManager();

        /// Creates and initializes the instance manager.
        static std::shared_ptr<VulkanInstanceManager> new_shared(std::string_view app_name, Version app_version);

        std::vector<vk::PhysicalDevice> enumerate_physical_devices() const;

      private:
        std::shared_ptr<spdlog::logger> m_logger;
        vk::DispatchLoaderDynamic       m_dispatch  = {};
        vk::Instance                    m_instance  = {};
        vk::DebugUtilsMessengerEXT      m_messenger = {};

        static VkBool32 VKAPI_CALL debug_utils_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
                                                        VkDebugUtilsMessageTypeFlagsEXT             type,
                                                        const VkDebugUtilsMessengerCallbackDataEXT *p_data,
                                                        void                                       *p_user_data);
    };
} // namespace engine
