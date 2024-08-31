#include "instance_manager.hpp"
#include "exceptions.hpp"
#include <GLFW/glfw3.h>
#include <span>
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

VulkanInstanceManager::VulkanInstanceManager(std::string_view app_name, Version app_version)
{
    if (!glfwInit())
        throw GlfwException("Failed to initialize GLFW");
    m_dispatch.init();

    std::vector<const char *> extensions = {};
    std::vector<const char *> layers     = {};

    get_glfw_extensions(extensions);

    if constexpr (DEBUG_ASSERTIONS) {
        layers.push_back("VK_LAYER_KHRONOS_validation");
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    auto unavailable_extensions = get_unavailable_instance_extensions(extensions);
    auto unavailable_layers     = get_unavailable_instance_layers(layers);

    uint32_t app_version_u32 =
        vk::makeApiVersion(app_version.variant, app_version.major, app_version.minor, app_version.patch);
    constexpr uint32_t engine_version_u32 =
        vk::makeApiVersion(0, ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_PATCH);

    vk::ApplicationInfo application_info(app_name.data(), app_version_u32, ENGINE_NAME, engine_version_u32,
                                         vk::ApiVersion13);

    vk::DebugUtilsMessengerCreateInfoEXT messenger_create_info(
        {}, MESSENGER_SEVERITY_FLAGS, MESSENGER_TYPE_FLAGS, [](...) {}, this);

    vk::InstanceCreateInfo instance_create_info({}, &application_info, layers.size(), layers.data(), extensions.size(),
                                                extensions.data());

    auto [result, instance] = vk::createInstance(instance_create_info, nullptr, m_dispatch);

    if (result != vk::Result::eSuccess)
        throw VulkanException((uint32_t)result, "Failed to initialize a Vulkan Instance");

    m_instance = instance;
}

VulkanInstanceManager::~VulkanInstanceManager() { }
