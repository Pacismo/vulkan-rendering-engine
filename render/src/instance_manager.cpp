#include "instance_manager.hpp"
#include "device_manager.hpp"
#include "exceptions.hpp"
#include "logger.hpp"
#include <GLFW/glfw3.h>
#include <memory>
#include <span>
#include <spdlog/spdlog.h>
#include <vector>
#include <vulkan/vulkan.hpp>

using std::vector, std::span, std::string_view;

namespace engine
{
    class VulkanInstanceManager::Deleter
    {
      public:
        void operator()(VulkanInstanceManager *ptr) { delete ptr; }
    };

    static constexpr vk::DebugUtilsMessageSeverityFlagsEXT MESSENGER_SEVERITY_FLAGS =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;

    static constexpr vk::DebugUtilsMessageTypeFlagsEXT MESSENGER_TYPE_FLAGS =
        vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding | vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
        | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;

    static void get_glfw_extensions(vector<const char *> &out)
    {
        uint32_t count = 0;
        auto     data  = glfwGetRequiredInstanceExtensions(&count);

        if (!data)
            throw GlfwException("Vulkan is not available on this machine");

        out.reserve(out.size() + count);
        for (uint32_t i = 0; i < count; ++i)
            out.push_back(data[i]);
    }

    static vector<const char *> get_unavailable_instance_extensions(span<const char *> extensions)
    {
        auto available_extensions = vk::enumerateInstanceExtensionProperties();

        vector<const char *> unavailable = {};

        for (auto extension : extensions) {
            bool found = false;

            for (auto &available : available_extensions)
                if (strcmp(available.extensionName, extension) == 0) {
                    found = false;
                    break;
                }

            if (!found)
                unavailable.push_back(extension);
        }

        return unavailable;
    }

    static vector<const char *> get_unavailable_instance_layers(span<const char *> layers)
    {
        auto available_layers = vk::enumerateInstanceLayerProperties();

        vector<const char *> unavailable = {};

        for (auto layer : layers) {
            bool found = false;

            for (auto &available : available_layers)
                if (strcmp(available.layerName, layer) == 0) {
                    found = false;
                    break;
                }

            if (!found)
                unavailable.push_back(layer);
        }

        return unavailable;
    }

    VkBool32 VKAPI_PTR VulkanInstanceManager::debug_utils_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
                                                                   VkDebugUtilsMessageTypeFlagsEXT             type,
                                                                   const VkDebugUtilsMessengerCallbackDataEXT *p_data,
                                                                   void *p_user_data)
    {
        auto        p_vim = (VulkanInstanceManager *)p_user_data;
        string_view message_type;

        if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
            message_type = "General";
        if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT)
            message_type = "Device Address Binding";
        if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
            message_type = "Performance";
        if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
            message_type = "Validation";

        switch ((vk::DebugUtilsMessageSeverityFlagBitsEXT)severity) {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
            p_vim->m_logger->info("Vulkan Debug Utils (Verbose/{}): {}\n{}", message_type, p_data->pMessageIdName,
                                  p_data->pMessage);
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
            p_vim->m_logger->trace("Vulkan Debug Utils (Info/{}): {}\n{}", message_type, p_data->pMessageIdName,
                                   p_data->pMessage);
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
            p_vim->m_logger->warn("Vulkan Debug Utils (Warning/{}): {}\n{}", message_type, p_data->pMessageIdName,
                                  p_data->pMessage);
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
            p_vim->m_logger->error("Vulkan Debug Utils (Error/{}): {}\n{}", message_type, p_data->pMessageIdName,
                                   p_data->pMessage);
            break;
        }
        return VK_FALSE;
    }

    VulkanInstanceManager::Shared VulkanInstanceManager::new_shared(string_view app_name, Version app_version)
    {
        return {new VulkanInstanceManager(app_name, app_version), Deleter()};
    }

    span<const vk::PhysicalDevice> VulkanInstanceManager::get_available_physical_devices() const
    {
        if (!m_instance)
            throw Exception("Not initialized");

        return m_available_devices;
    }

    std::span<const vk::PhysicalDevice> VulkanInstanceManager::get_supported_rendering_devices() const
    {
        if (!m_instance)
            throw Exception("Not initialized");
        return m_supported_rendering_devices;
    }

    VulkanInstanceManager::VulkanInstanceManager(string_view app_name, Version app_version)
        : m_logger(get_logger())
    {

        if (!glfwInit())
            throw GlfwException("Failed to initialize GLFW");
        if (!glfwVulkanSupported())
            throw VulkanException((uint32_t)vk::Result::eErrorUnknown, "Vulkan is not supported on this device");

        m_dispatch.init();

        vector<const char *> extensions = {};
        vector<const char *> layers     = {};

        get_glfw_extensions(extensions);

        if constexpr (DEBUG_ASSERTIONS) {
            layers.push_back("VK_LAYER_KHRONOS_validation");
            extensions.push_back(vk::EXTDebugUtilsExtensionName);

            m_logger->info("Debug assertions are enabled - loading debug utilities...");
        }

        auto unavailable_extensions = get_unavailable_instance_extensions(extensions);
        auto unavailable_layers     = get_unavailable_instance_layers(layers);

        uint32_t app_version_u32 =
            vk::makeApiVersion(app_version.variant, app_version.major, app_version.minor, app_version.patch);
        constexpr uint32_t engine_version_u32 =
            vk::makeApiVersion(0, ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_PATCH);

        vk::ApplicationInfo application_info {
            .pApplicationName   = app_name.data(),
            .applicationVersion = app_version_u32,
            .pEngineName        = ENGINE_NAME,
            .engineVersion      = engine_version_u32,
            .apiVersion         = vk::ApiVersion13,
        };

        vk::DebugUtilsMessengerCreateInfoEXT messenger_create_info {
            .messageSeverity = MESSENGER_SEVERITY_FLAGS,
            .messageType     = MESSENGER_TYPE_FLAGS,
            .pfnUserCallback = debug_utils_callback,
            .pUserData       = (void *)this,
        };

        vk::InstanceCreateInfo instance_create_info {
            .pNext                   = DEBUG_ASSERTIONS ? &messenger_create_info : nullptr,
            .pApplicationInfo        = &application_info,
            .enabledLayerCount       = (uint32_t)layers.size(),
            .ppEnabledLayerNames     = layers.data(),
            .enabledExtensionCount   = (uint32_t)extensions.size(),
            .ppEnabledExtensionNames = extensions.data(),
        };

        m_instance = vk::createInstance(instance_create_info, nullptr, m_dispatch);

        m_logger->info("Created Vulkan Instance");

        m_dispatch.init(m_instance);

        if constexpr (DEBUG_ASSERTIONS) {
            m_messenger = m_instance.createDebugUtilsMessengerEXT(messenger_create_info, nullptr, m_dispatch);

            m_logger->info("Created debug utils messenger");
        }

        {
            auto available_devices = m_instance.enumeratePhysicalDevices();

            span<const char *> required_device_extensions = RenderDeviceManager::get_required_device_extensions();
            vector<vk::PhysicalDevice> supported_devices  = {};

            // Get a list of supported devices
            for (vk::PhysicalDevice device : available_devices) {
                auto available_extensions = device.enumerateDeviceExtensionProperties();

                bool supported = true;

                for (auto extension : required_device_extensions) {
                    bool found = false;
                    for (auto &available : available_extensions)
                        if (strcmp(extension, available.extensionName.data()) == 0) {
                            found = true;
                            break;
                        }

                    supported &= found;
                }

                if (supported)
                    supported_devices.push_back(device);
            }

            m_available_devices           = std::move(available_devices);
            m_supported_rendering_devices = std::move(supported_devices);
        }
    }

    VulkanInstanceManager::~VulkanInstanceManager()
    {
        if (m_messenger)
            m_instance.destroyDebugUtilsMessengerEXT(m_messenger, nullptr, m_dispatch);

        if (m_instance)
            m_instance.destroy();

        m_logger->info("Cleaned up Vulkan instance manager");

        m_available_devices.clear();
        m_supported_rendering_devices.clear();
        m_messenger = nullptr;
        m_instance  = nullptr;
    }
} // namespace engine
