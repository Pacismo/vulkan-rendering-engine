#pragma once
#include <memory>
#include <span>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

namespace engine
{
    struct Queue
    {
        uint32_t  index;
        vk::Queue handle;
    };

    class RenderDeviceManager final
    {
        friend class Window;
        friend class VulkanInstanceManager;
        friend class RenderManager;

        using SharedInstanceManager = std::shared_ptr<class VulkanInstanceManager>;

        class Deleter;

      public:
        using Shared = std::shared_ptr<RenderDeviceManager>;

        static Shared new_shared(SharedInstanceManager instance_manager, vk::PhysicalDevice physical_device);

      private:
        RenderDeviceManager(SharedInstanceManager instance_manager, vk::PhysicalDevice physical_device);
        ~RenderDeviceManager();

        RenderDeviceManager(const RenderDeviceManager &)            = delete;
        RenderDeviceManager(RenderDeviceManager &&)                 = delete;
        RenderDeviceManager &operator=(const RenderDeviceManager &) = delete;
        RenderDeviceManager &operator=(RenderDeviceManager &&)      = delete;

        static std::span<const char *> get_required_device_extensions();

        std::shared_ptr<spdlog::logger>              m_logger           = {};
        std::shared_ptr<class VulkanInstanceManager> m_instance_manager = {};
        vk::DispatchLoaderDynamic                    m_dispatch         = {};
        vk::PhysicalDevice                           m_physical_device  = {};
        vk::Device                                   m_device           = {};
        Queue                                        m_graphics_queue   = {};
        Queue                                        m_present_queue    = {};
    };
} // namespace engine