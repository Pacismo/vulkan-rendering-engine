#include "backend/device_manager.hpp"
#include "backend/instance_manager.hpp"
#include "exceptions.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <optional>
#include <set>
#include <tuple>

using std::tuple, std::optional, std::set, std::vector, std::array, std::span;

static array<const char *, 1>     REQUIRED_DEVICE_EXTENSIONS = {vk::KHRSwapchainExtensionName};
static vk::PhysicalDeviceFeatures REQUIRED_DEVICE_FEATURES   = {};

namespace engine
{
    SingleTimeCommandBuffer::SingleTimeCommandBuffer(RenderDeviceManager *manager, vk::Queue queue,
                                                     vk::CommandBuffer buffer)
        : p_manager(manager)
        , queue(queue)
        , buffer(buffer)
    {
        buffer.begin(vk::CommandBufferBeginInfo {.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    }

    SingleTimeCommandBuffer::~SingleTimeCommandBuffer()
    {
        if (p_manager)
            p_manager->device.freeCommandBuffers(p_manager->command_pool, buffer);
    }

    void SingleTimeCommandBuffer::submit()
    {
        buffer.end();

        queue.submit(vk::SubmitInfo {
            .commandBufferCount = 1,
            .pCommandBuffers    = &buffer,
        });
        queue.waitIdle();
        p_manager->device.freeCommandBuffers(p_manager->command_pool, buffer);

        p_manager = nullptr;
        queue     = nullptr;
        buffer    = nullptr;
    }

    static tuple<optional<uint32_t>, optional<uint32_t>> get_gp_queue_indices(vk::Instance       instance,
                                                                              vk::PhysicalDevice device)
    {
        auto        queues     = device.getQueueFamilyProperties();
        const char *glfw_error = nullptr;

        optional<uint32_t> graphics;
        optional<uint32_t> present;

        for (uint32_t i = 0; i < queues.size(); ++i) {
            if (queues[i].queueFlags & vk::QueueFlagBits::eGraphics)
                graphics = i;

            if (glfwGetPhysicalDevicePresentationSupport(instance, device, i))
                present = i;
            else if (int glfw_error_code = glfwGetError(&glfw_error))
                throw GlfwException("Failed to query device presentation support", glfw_error_code, glfw_error);

            if (graphics.has_value() && present.has_value())
                if (graphics.value() == present.value())
                    break;
        }

        return {graphics, present};
    }

    class RenderDeviceManager::Deleter
    {
      public:
        void operator()(RenderDeviceManager *ptr) { delete ptr; }
    };

    SingleTimeCommandBuffer engine::RenderDeviceManager::single_time_command()
    {
        return SingleTimeCommandBuffer(this, graphics_queue.handle,
                                       device.allocateCommandBuffers(vk::CommandBufferAllocateInfo {
                                           .commandPool        = command_pool,
                                           .level              = vk::CommandBufferLevel::ePrimary,
                                           .commandBufferCount = 1,
                                       })[0]);
    }

    RenderDeviceManager::RenderDeviceManager(SharedInstanceManager instance_manager, vk::PhysicalDevice physical_device)
        : logger(instance_manager->logger)
        , instance_manager(instance_manager)
        , dispatch(instance_manager->dispatch)
        , physical_device(physical_device)
    {
        // Make sure that the device passed is available.
        if constexpr (DEBUG_ASSERTIONS) {
            auto devices =
                instance_manager->get_supported_rendering_devices(REQUIRED_DEVICE_EXTENSIONS, REQUIRED_DEVICE_FEATURES);

            auto idx = std::find(devices.begin(), devices.end(), physical_device);
            if (idx == devices.end())
                throw Exception("The device passed to the constructor either does not support the required extensions "
                                "or is not available");
        }

        logger->info("Creating render device using {}", physical_device.getProperties().deviceName.data());

        auto [graphics_queue_index, present_queue_index] =
            get_gp_queue_indices(instance_manager->instance, physical_device);

        if (!graphics_queue_index.has_value())
            throw VulkanException((uint32_t)vk::Result::eErrorUnknown, "Could not find a graphics queue");
        if (!present_queue_index.has_value())
            throw VulkanException((uint32_t)vk::Result::eErrorUnknown, "Could not find a present queue");

        set<uint32_t> queue_indices = {graphics_queue_index.value(), present_queue_index.value()};

        span<const char *> device_extensions = REQUIRED_DEVICE_EXTENSIONS;

        vector<vk::DeviceQueueCreateInfo> queue_create_infos;
        float                             queue_priority = 1.0;

        for (auto index : queue_indices)
            queue_create_infos.push_back(vk::DeviceQueueCreateInfo {
                .flags            = {},
                .queueFamilyIndex = index,
                .queueCount       = 1,
                .pQueuePriorities = &queue_priority,
            });

        vk::DeviceCreateInfo dci = {
            .queueCreateInfoCount    = (uint32_t)queue_create_infos.size(),
            .pQueueCreateInfos       = queue_create_infos.data(),
            .enabledLayerCount       = 0,
            .ppEnabledLayerNames     = nullptr,
            .enabledExtensionCount   = (uint32_t)device_extensions.size(),
            .ppEnabledExtensionNames = device_extensions.data(),
            .pEnabledFeatures        = &REQUIRED_DEVICE_FEATURES,
        };

        device = physical_device.createDevice(dci);
        logger->info("Created device");

        graphics_queue = Queue {
            .index  = graphics_queue_index.value(),
            .handle = device.getQueue(graphics_queue_index.value(), 0),
        };
        present_queue = Queue {
            .index  = present_queue_index.value(),
            .handle = device.getQueue(present_queue_index.value(), 0),
        };

        vk::CommandPoolCreateInfo cpi = {
            .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = graphics_queue.index,
        };

        command_pool = device.createCommandPool(cpi);

        logger->info("Selected queue family {} for graphics queue ({:8X})", graphics_queue.index,
                     (size_t)(VkQueue)graphics_queue.handle);
        logger->info("Selected queue family {} for present queue ({:8X})", present_queue.index,
                     (size_t)(VkQueue)present_queue.handle);
    }

    RenderDeviceManager::~RenderDeviceManager()
    {
        device.destroyCommandPool(command_pool);
        device.destroy();
        logger->info("Destroyed device");

        graphics_queue  = {};
        present_queue   = {};
        device          = nullptr;
        physical_device = nullptr;

        instance_manager = nullptr;
    }

    span<const char *> RenderDeviceManager::get_required_device_extensions()
    {
        return REQUIRED_DEVICE_EXTENSIONS;
    }

    const vk::PhysicalDeviceFeatures &RenderDeviceManager::get_required_device_features()
    {
        return REQUIRED_DEVICE_FEATURES;
    }

    RenderDeviceManager::Shared RenderDeviceManager::new_shared(SharedInstanceManager instance_manager,
                                                                vk::PhysicalDevice    physical_device)
    {
        return {new RenderDeviceManager(instance_manager, physical_device), Deleter()};
    }

    vk::Format engine::RenderDeviceManager::find_supported_format(std::span<const vk::Format> formats,
                                                                  vk::ImageTiling             tiling,
                                                                  vk::FormatFeatureFlags      features) const
    {
        for (auto format : formats) {
            vk::FormatProperties properties = physical_device.getFormatProperties(format);

            if ((tiling == vk::ImageTiling::eLinear) && (properties.linearTilingFeatures & features))
                return format;
            else if ((tiling == vk::ImageTiling::eOptimal) && (properties.optimalTilingFeatures & features))
                return format;
        }

        throw engine::VulkanException((uint32_t)vk::Result::eErrorFormatNotSupported,
                                      "Failed to find a supported format");
    }
} // namespace engine
