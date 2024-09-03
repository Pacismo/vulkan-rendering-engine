#include "device_manager.hpp"
#include "exceptions.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <instance_manager.hpp>
#include <optional>
#include <set>
#include <tuple>

using std::tuple, std::move, std::forward, std::optional, std::set, std::vector, std::array, std::span;

static array<const char *, 1> REQUIRED_DEVICE_EXTENSIONS = {vk::KHRSwapchainExtensionName};

namespace engine
{
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

    static optional<uint32_t> get_transer_queue_index(vk::PhysicalDevice device) { }

    class RenderDeviceManager::Deleter
    {
      public:
        void operator()(RenderDeviceManager *ptr) { delete ptr; }
    };

    RenderDeviceManager::RenderDeviceManager(SharedInstanceManager instance_manager, vk::PhysicalDevice physical_device)
        : m_logger(instance_manager->m_logger)
        , m_instance_manager(move(instance_manager))
        , m_dispatch(m_instance_manager->m_dispatch)
        , m_physical_device(physical_device)
    {
        // Make sure that the device passed is available.
        if constexpr (DEBUG_ASSERTIONS) {
            auto devices = m_instance_manager->get_supported_rendering_devices();

            auto idx = std::find(devices.begin(), devices.end(), physical_device);
            if (idx == devices.end())
                throw Exception("The device passed to the constructor either does not support the required extensions "
                                "or is not available");
        }

        m_logger->info("Creating render device using {}", physical_device.getProperties().deviceName.data());

        auto [graphics_queue_index, present_queue_index] =
            get_gp_queue_indices(m_instance_manager->m_instance, physical_device);

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

        vk::PhysicalDeviceFeatures features = {};

        {
            vk::DeviceCreateInfo dci = {
                .queueCreateInfoCount    = (uint32_t)queue_create_infos.size(),
                .pQueueCreateInfos       = queue_create_infos.data(),
                .enabledLayerCount       = 0,
                .ppEnabledLayerNames     = nullptr,
                .enabledExtensionCount   = (uint32_t)device_extensions.size(),
                .ppEnabledExtensionNames = device_extensions.data(),
                .pEnabledFeatures        = &features,
            };

            auto [result, device] = m_physical_device.createDevice(dci);

            if (result != vk::Result::eSuccess)
                throw VulkanException((uint32_t)result, "Failed to create device");
            m_logger->info("Created device");

            m_device = device;
        }

        m_graphics_queue = Queue {
            .index  = graphics_queue_index.value(),
            .handle = m_device.getQueue(graphics_queue_index.value(), 0),
        };
        m_present_queue = Queue {
            .index  = present_queue_index.value(),
            .handle = m_device.getQueue(present_queue_index.value(), 0),
        };

        m_logger->info("Selected queue family {} for graphics queue ({:8X})", m_graphics_queue.index,
                       (size_t)(VkQueue)m_graphics_queue.handle);
        m_logger->info("Selected queue family {} for present queue ({:8X})", m_present_queue.index,
                       (size_t)(VkQueue)m_present_queue.handle);
    }

    RenderDeviceManager::~RenderDeviceManager()
    {
        if (m_device)
            m_device.destroy();
        m_logger->info("Destroyed device {:08X}", (size_t)(VkDevice)m_device);

        m_graphics_queue  = {};
        m_present_queue   = {};
        m_device          = nullptr;
        m_physical_device = nullptr;

        m_instance_manager = nullptr;
    }

    span<const char *> RenderDeviceManager::get_required_device_extensions()
    {
        return REQUIRED_DEVICE_EXTENSIONS;
    }

    RenderDeviceManager::Shared RenderDeviceManager::new_shared(SharedInstanceManager instance_manager,
                                                                vk::PhysicalDevice    physical_device)
    {
        return {new RenderDeviceManager(instance_manager, physical_device), Deleter()};
    }
} // namespace engine
