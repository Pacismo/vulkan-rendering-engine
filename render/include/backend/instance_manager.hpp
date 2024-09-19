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
    /// Manages the data pertaining to a Vulkan instance.
    ///
    /// May be shared.
    class VulkanInstanceManager final
    {
        friend class RenderDeviceManager;
        friend class VulkanBackend;

        class Deleter;

      public:
        using Shared = std::shared_ptr<VulkanInstanceManager>;

        /// Creates and initializes the instance manager.
        static Shared new_shared(std::string_view app_name, Version app_version);

        std::span<const vk::PhysicalDevice> get_available_physical_devices() const;
        std::vector<vk::PhysicalDevice> get_supported_rendering_devices(std::span<const char *> extensions, const vk::PhysicalDeviceFeatures &features) const;

        std::shared_ptr<spdlog::logger> logger                      = {};
        vk::DispatchLoaderDynamic       dispatch                    = {};
        vk::Instance                    instance                    = {};
        vk::DebugUtilsMessengerEXT      messenger                   = {};
        std::vector<vk::PhysicalDevice> available_devices           = {};

      private:
        VulkanInstanceManager(std::string_view app_name, Version app_version);
        ~VulkanInstanceManager();

        VulkanInstanceManager(VulkanInstanceManager &&)                 = delete;
        VulkanInstanceManager(const VulkanInstanceManager &)            = delete;
        VulkanInstanceManager &operator=(VulkanInstanceManager &&)      = delete;
        VulkanInstanceManager &operator=(const VulkanInstanceManager &) = delete;

        static VkBool32 VKAPI_CALL debug_utils_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
                                                        VkDebugUtilsMessageTypeFlagsEXT             type,
                                                        const VkDebugUtilsMessengerCallbackDataEXT *p_data,
                                                        void                                       *p_user_data);
    };
} // namespace engine
