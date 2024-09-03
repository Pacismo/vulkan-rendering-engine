#pragma once

#include "exceptions.hpp"
#include "version.hpp"

#include <memory>
#include <span>
#include <spdlog/spdlog.h>
#include <string_view>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace engine
{
    class VulkanInstanceManager final
    {
        friend class RenderDeviceManager;
        friend class RenderManager;
        friend class Window;

        class Deleter;

      public:
        using Shared = std::shared_ptr<VulkanInstanceManager>;

        /// Creates and initializes the instance manager.
        static Shared new_shared(std::string_view app_name, Version app_version);

        std::span<const vk::PhysicalDevice> get_available_physical_devices() const;
        std::span<const vk::PhysicalDevice> get_supported_rendering_devices() const;

      private:
        VulkanInstanceManager(std::string_view app_name, Version app_version);
        ~VulkanInstanceManager();

        VulkanInstanceManager(VulkanInstanceManager &&)                 = delete;
        VulkanInstanceManager(const VulkanInstanceManager &)            = delete;
        VulkanInstanceManager &operator=(VulkanInstanceManager &&)      = delete;
        VulkanInstanceManager &operator=(const VulkanInstanceManager &) = delete;

        std::shared_ptr<spdlog::logger> m_logger                      = {};
        vk::DispatchLoaderDynamic       m_dispatch                    = {};
        vk::Instance                    m_instance                    = {};
        vk::DebugUtilsMessengerEXT      m_messenger                   = {};
        std::vector<vk::PhysicalDevice> m_available_devices           = {};
        std::vector<vk::PhysicalDevice> m_supported_rendering_devices = {};

        static VkBool32 VKAPI_CALL debug_utils_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
                                                        VkDebugUtilsMessageTypeFlagsEXT             type,
                                                        const VkDebugUtilsMessengerCallbackDataEXT *p_data,
                                                        void                                       *p_user_data);
    };
} // namespace engine
