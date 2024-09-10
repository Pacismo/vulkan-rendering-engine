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

    /// Manages the data pertaining to the rendering device.
    ///
    /// May be shared.
    class RenderDeviceManager final
    {
        friend class VulkanBackend;

        using SharedInstanceManager = std::shared_ptr<class VulkanInstanceManager>;

        class Deleter;

      public:
        using Shared = std::shared_ptr<RenderDeviceManager>;

        static Shared new_shared(SharedInstanceManager instance_manager, vk::PhysicalDevice physical_device);

        std::shared_ptr<spdlog::logger>              logger           = {};
        std::shared_ptr<class VulkanInstanceManager> instance_manager = {};
        vk::DispatchLoaderDynamic                    dispatch         = {};
        vk::PhysicalDevice                           physical_device  = {};
        vk::Device                                   device           = {};
        Queue                                        graphics_queue   = {};
        Queue                                        present_queue    = {};

      private:
        RenderDeviceManager(SharedInstanceManager instance_manager, vk::PhysicalDevice physical_device);
        ~RenderDeviceManager();

        RenderDeviceManager(const RenderDeviceManager &)            = delete;
        RenderDeviceManager(RenderDeviceManager &&)                 = delete;
        RenderDeviceManager &operator=(const RenderDeviceManager &) = delete;
        RenderDeviceManager &operator=(RenderDeviceManager &&)      = delete;

      public:
        static std::span<const char *> get_required_device_extensions();
    };
} // namespace engine