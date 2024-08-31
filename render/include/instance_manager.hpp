#pragma once

#include "exceptions.hpp"
#include "version.hpp"
#include <string_view>
#include <vulkan/vulkan.hpp>

namespace engine
{
    class VulkanInstanceManager final
    {
      public:
        /// Creates and initializes the instance manager.
        VulkanInstanceManager(std::string_view app_name, Version app_version);
        ~VulkanInstanceManager();

      private:
        vk::DispatchLoaderDynamic  m_dispatch        = {};
        vk::Instance               m_instance        = {};
        vk::DebugUtilsMessengerEXT m_messenger       = {};
        vk::PhysicalDevice         m_physical_device = {};
        vk::Device                 m_device          = {};
    };
} // namespace engine
