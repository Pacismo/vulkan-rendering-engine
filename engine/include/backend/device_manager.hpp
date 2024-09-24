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

    class SingleTimeCommandBuffer
    {
        class RenderDeviceManager *p_manager;
        vk::Queue                  queue;
        vk::CommandBuffer          buffer;

      public:
        inline vk::CommandBuffer *operator->() { return &buffer; }

        inline operator vk::CommandBuffer() { return buffer; }

        void submit();

        SingleTimeCommandBuffer(class RenderDeviceManager *manager, vk::Queue queue, vk::CommandBuffer buffer);

        ~SingleTimeCommandBuffer();
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

        vk::Format find_supported_format(std::span<const vk::Format> formats, vk::ImageTiling tiling,
                                         vk::FormatFeatureFlags features) const;

        std::shared_ptr<spdlog::logger>              logger           = {};
        std::shared_ptr<class VulkanInstanceManager> instance_manager = {};
        std::weak_ptr<class VulkanAllocator>         allocator        = {}; // Weak to prevent cyclic reference
        vk::DispatchLoaderDynamic                    dispatch         = {};
        vk::PhysicalDevice                           physical_device  = {};
        vk::Device                                   device           = {};
        Queue                                        graphics_queue   = {};
        Queue                                        present_queue    = {};
        vk::CommandPool                              command_pool     = {};

        SingleTimeCommandBuffer single_time_command();

      private:
        RenderDeviceManager(SharedInstanceManager instance_manager, vk::PhysicalDevice physical_device);
        ~RenderDeviceManager();

        RenderDeviceManager(const RenderDeviceManager &)            = delete;
        RenderDeviceManager(RenderDeviceManager &&)                 = delete;
        RenderDeviceManager &operator=(const RenderDeviceManager &) = delete;
        RenderDeviceManager &operator=(RenderDeviceManager &&)      = delete;

      public:
        static std::span<const char *>           get_required_device_extensions();
        static const vk::PhysicalDeviceFeatures &get_required_device_features();
    };
} // namespace engine