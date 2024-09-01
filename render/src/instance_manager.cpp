#include "instance_manager.hpp"
#include "exceptions.hpp"
#include <GLFW/glfw3.h>
#include <memory>
#include <span>
#include <spdlog/spdlog.h>
#include <vector>
#include <vulkan/vulkan.hpp>

using namespace engine;

static constexpr vk::DebugUtilsMessageSeverityFlagsEXT MESSENGER_SEVERITY_FLAGS =
    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
    | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;

static constexpr vk::DebugUtilsMessageTypeFlagsEXT MESSENGER_TYPE_FLAGS =
    vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding | vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
    | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;

static void get_glfw_extensions(std::vector<const char *> &out)
{
    uint32_t count = 0;
    auto     data  = glfwGetRequiredInstanceExtensions(&count);

    if (!data)
        throw GlfwException("Vulkan is not available on this machine");

    out.reserve(out.size() + count);
    for (uint32_t i = 0; i < count; ++i)
        out.push_back(data[i]);
}

static std::vector<const char *> get_unavailable_instance_extensions(std::span<const char *> extensions)
{
    auto [result, available_extensions] = vk::enumerateInstanceExtensionProperties();

    if (result != vk::Result::eSuccess)
        throw VulkanException((uint32_t)result, "Failed to get a list of Vulkan Instance extensions");

    std::vector<const char *> unavailable = {};

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

static std::vector<const char *> get_unavailable_instance_layers(std::span<const char *> layers)
{
    auto [result, available_layers] = vk::enumerateInstanceLayerProperties();

    if (result != vk::Result::eSuccess)
        throw VulkanException((uint32_t)result, "Failed to get a list of Vulkan Instance layers");

    std::vector<const char *> unavailable = {};

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

static std::shared_ptr<spdlog::logger> get_logger()
{
    if (auto logger = spdlog::get("render"))
        return logger;

    const auto                     &sinks  = spdlog::default_logger()->sinks();
    std::shared_ptr<spdlog::logger> logger = std::make_shared<spdlog::logger>("render", sinks.begin(), sinks.end());
    spdlog::initialize_logger(logger);
    return logger;
}

VkBool32 VKAPI_PTR VulkanInstanceManager::debug_utils_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
                                                               VkDebugUtilsMessageTypeFlagsEXT             type,
                                                               const VkDebugUtilsMessengerCallbackDataEXT *p_data,
                                                               void                                       *p_user_data)
{
    auto             p_vim = (VulkanInstanceManager *)p_user_data;
    std::string_view message_type;

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

std::shared_ptr<VulkanInstanceManager> engine::VulkanInstanceManager::new_shared(std::string_view app_name,
                                                                                 Version          app_version)
{
    return std::make_shared<VulkanInstanceManager>(app_name, app_version);
}

std::vector<vk::PhysicalDevice> engine::VulkanInstanceManager::enumerate_physical_devices() const
{
    if (!m_instance)
        throw Exception("Not initialized");

    auto [result, devices] = m_instance.enumeratePhysicalDevices();

    if (result != vk::Result::eSuccess)
        throw VulkanException((uint32_t)result, "Failed to enumerate physical devices");

    return devices;
}

VulkanInstanceManager::VulkanInstanceManager(std::string_view app_name, Version app_version)
    : m_logger(get_logger())
{

    if (!glfwInit())
        throw GlfwException("Failed to initialize GLFW");
    if (!glfwVulkanSupported())
        throw VulkanException((uint32_t)vk::Result::eErrorUnknown, "Vulkan is not supported on this device");

    m_dispatch.init();

    std::vector<const char *> extensions = {};
    std::vector<const char *> layers     = {};

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

    {
        auto [result, instance] = vk::createInstance(instance_create_info, nullptr, m_dispatch);

        if (result != vk::Result::eSuccess)
            throw VulkanException((uint32_t)result, "Failed to initialize a Vulkan Instance");

        m_instance = instance;

        m_logger->info("Created Vulkan Instance");
    }

    m_dispatch.init(m_instance);

    if constexpr (DEBUG_ASSERTIONS) {
        auto [result, dmessenger] = m_instance.createDebugUtilsMessengerEXT(messenger_create_info, nullptr, m_dispatch);

        if (result != vk::Result::eSuccess)
            throw VulkanException((uint32_t)result, "Failed to initialize a Vulkan Debug Messenger");

        m_messenger = dmessenger;

        m_logger->info("Created debug utils messenger");
    }
}

VulkanInstanceManager::~VulkanInstanceManager()
{
    if (m_messenger)
        m_instance.destroyDebugUtilsMessengerEXT(m_messenger, nullptr, m_dispatch);

    if (m_instance)
        m_instance.destroy();

    m_logger->info("Cleaned up Vulkan instance");
}
