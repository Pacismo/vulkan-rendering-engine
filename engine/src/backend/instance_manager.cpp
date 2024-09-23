#include "backend/instance_manager.hpp"
#include "backend/device_manager.hpp"
#include "exceptions.hpp"
#include "logger.hpp"
#include <GLFW/glfw3.h>
#include <memory>
#include <span>
#include <spdlog/spdlog.h>
#include <vector>
#include <vulkan/vulkan.hpp>

#if DEBUG_ASSERTIONS
#    if defined(_CrtDbgBreak)
/// CRT debug break
#        define DEBUG_BREAK _CrtDbgBreak()
#    elif not defined(WIN32)
#        include <signal.h>
/// Non-Windows debug break (raise SIGTRAP)
#        define DEBUG_BREAK raise(SIGTRAP)
#    else
/// Non-CRT Windows is not supported
#        define DEBUG_BREAK
#    endif
#else
/// No operation
#    define DEBUG_BREAK
#endif

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
            p_vim->logger->info("Vulkan Debug Utils (Verbose/{}): {}\n{}", message_type, p_data->pMessageIdName,
                                p_data->pMessage);
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
            p_vim->logger->trace("Vulkan Debug Utils (Info/{}): {}\n{}", message_type, p_data->pMessageIdName,
                                 p_data->pMessage);
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
            p_vim->logger->warn("Vulkan Debug Utils (Warning/{}): {}\n{}", message_type, p_data->pMessageIdName,
                                p_data->pMessage);
            //DEBUG_BREAK;
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
            p_vim->logger->error("Vulkan Debug Utils (Error/{}): {}\n{}", message_type, p_data->pMessageIdName,
                                 p_data->pMessage);
            DEBUG_BREAK;
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
        if (!instance)
            throw Exception("Not initialized");

        return available_devices;
    }

    vector<vk::PhysicalDevice> VulkanInstanceManager::get_supported_rendering_devices(
        std::span<const char *> extensions, const vk::PhysicalDeviceFeatures &features) const
    {
        vector<vk::PhysicalDevice> supported_devices = {};

        // Get a list of supported devices
        for (vk::PhysicalDevice device : available_devices) {
            auto available_extensions = device.enumerateDeviceExtensionProperties();
            auto available_features   = device.getFeatures();

            bool extensions_supported = true;

            for (auto extension : extensions) {
                bool found = false;
                for (auto &available : available_extensions)
                    if (strcmp(extension, available.extensionName.data()) == 0) {
                        found = true;
                        break;
                    }

                if (!found) {
                    extensions_supported = false;
                    break;
                }
            }

            // request -> available => !request || available
            // 
            // In other words, request == 0 or available == 1
#define IS_FEATURE_SUPPORTED(feature) ((features.feature == 0) || (available_features.feature == 1))
            bool features_supported = IS_FEATURE_SUPPORTED(robustBufferAccess)                      //
                                   && IS_FEATURE_SUPPORTED(fullDrawIndexUint32)                     //
                                   && IS_FEATURE_SUPPORTED(imageCubeArray)                          //
                                   && IS_FEATURE_SUPPORTED(independentBlend)                        //
                                   && IS_FEATURE_SUPPORTED(geometryShader)                          //
                                   && IS_FEATURE_SUPPORTED(tessellationShader)                      //
                                   && IS_FEATURE_SUPPORTED(sampleRateShading)                       //
                                   && IS_FEATURE_SUPPORTED(dualSrcBlend)                            //
                                   && IS_FEATURE_SUPPORTED(logicOp)                                 //
                                   && IS_FEATURE_SUPPORTED(multiDrawIndirect)                       //
                                   && IS_FEATURE_SUPPORTED(drawIndirectFirstInstance)               //
                                   && IS_FEATURE_SUPPORTED(depthClamp)                              //
                                   && IS_FEATURE_SUPPORTED(depthBiasClamp)                          //
                                   && IS_FEATURE_SUPPORTED(fillModeNonSolid)                        //
                                   && IS_FEATURE_SUPPORTED(depthBounds)                             //
                                   && IS_FEATURE_SUPPORTED(wideLines)                               //
                                   && IS_FEATURE_SUPPORTED(largePoints)                             //
                                   && IS_FEATURE_SUPPORTED(alphaToOne)                              //
                                   && IS_FEATURE_SUPPORTED(multiViewport)                           //
                                   && IS_FEATURE_SUPPORTED(samplerAnisotropy)                       //
                                   && IS_FEATURE_SUPPORTED(textureCompressionETC2)                  //
                                   && IS_FEATURE_SUPPORTED(textureCompressionASTC_LDR)              //
                                   && IS_FEATURE_SUPPORTED(textureCompressionBC)                    //
                                   && IS_FEATURE_SUPPORTED(occlusionQueryPrecise)                   //
                                   && IS_FEATURE_SUPPORTED(pipelineStatisticsQuery)                 //
                                   && IS_FEATURE_SUPPORTED(vertexPipelineStoresAndAtomics)          //
                                   && IS_FEATURE_SUPPORTED(fragmentStoresAndAtomics)                //
                                   && IS_FEATURE_SUPPORTED(shaderTessellationAndGeometryPointSize)  //
                                   && IS_FEATURE_SUPPORTED(shaderImageGatherExtended)               //
                                   && IS_FEATURE_SUPPORTED(shaderStorageImageExtendedFormats)       //
                                   && IS_FEATURE_SUPPORTED(shaderStorageImageMultisample)           //
                                   && IS_FEATURE_SUPPORTED(shaderStorageImageReadWithoutFormat)     //
                                   && IS_FEATURE_SUPPORTED(shaderStorageImageWriteWithoutFormat)    //
                                   && IS_FEATURE_SUPPORTED(shaderUniformBufferArrayDynamicIndexing) //
                                   && IS_FEATURE_SUPPORTED(shaderSampledImageArrayDynamicIndexing)  //
                                   && IS_FEATURE_SUPPORTED(shaderStorageBufferArrayDynamicIndexing) //
                                   && IS_FEATURE_SUPPORTED(shaderStorageImageArrayDynamicIndexing)  //
                                   && IS_FEATURE_SUPPORTED(shaderClipDistance)                      //
                                   && IS_FEATURE_SUPPORTED(shaderCullDistance)                      //
                                   && IS_FEATURE_SUPPORTED(shaderFloat64)                           //
                                   && IS_FEATURE_SUPPORTED(shaderInt64)                             //
                                   && IS_FEATURE_SUPPORTED(shaderInt16)                             //
                                   && IS_FEATURE_SUPPORTED(shaderResourceResidency)                 //
                                   && IS_FEATURE_SUPPORTED(shaderResourceMinLod)                    //
                                   && IS_FEATURE_SUPPORTED(sparseBinding)                           //
                                   && IS_FEATURE_SUPPORTED(sparseResidencyBuffer)                   //
                                   && IS_FEATURE_SUPPORTED(sparseResidencyImage2D)                  //
                                   && IS_FEATURE_SUPPORTED(sparseResidencyImage3D)                  //
                                   && IS_FEATURE_SUPPORTED(sparseResidency2Samples)                 //
                                   && IS_FEATURE_SUPPORTED(sparseResidency4Samples)                 //
                                   && IS_FEATURE_SUPPORTED(sparseResidency8Samples)                 //
                                   && IS_FEATURE_SUPPORTED(sparseResidency16Samples)                //
                                   && IS_FEATURE_SUPPORTED(sparseResidencyAliased)                  //
                                   && IS_FEATURE_SUPPORTED(variableMultisampleRate)                 //
                                   && IS_FEATURE_SUPPORTED(inheritedQueries);                       //
#undef IS_FEATURE_SUPPORTED

            if (extensions_supported && features_supported)
                supported_devices.push_back(device);
        }

        return supported_devices;
    }

    VulkanInstanceManager::VulkanInstanceManager(string_view app_name, Version app_version)
        : logger(get_logger())
    {

        if (!glfwInit())
            throw GlfwException("Failed to initialize GLFW");
        if (!glfwVulkanSupported())
            throw VulkanException((uint32_t)vk::Result::eErrorUnknown, "Vulkan is not supported on this device");

        dispatch.init();

        vector<const char *> extensions = {};
        vector<const char *> layers     = {};

        get_glfw_extensions(extensions);

        if constexpr (DEBUG_ASSERTIONS) {
            layers.push_back("VK_LAYER_KHRONOS_validation");
            extensions.push_back(vk::EXTDebugUtilsExtensionName);

            logger->info("Debug assertions are enabled - loading debug utilities...");
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

        instance = vk::createInstance(instance_create_info, nullptr, dispatch);

        logger->info("Created Vulkan Instance");

        dispatch.init(instance);

        if constexpr (DEBUG_ASSERTIONS) {
            messenger = instance.createDebugUtilsMessengerEXT(messenger_create_info, nullptr, dispatch);

            logger->info("Created debug utils messenger");
        }

        available_devices = instance.enumeratePhysicalDevices();
    }

    VulkanInstanceManager::~VulkanInstanceManager()
    {
        if (messenger)
            instance.destroyDebugUtilsMessengerEXT(messenger, nullptr, dispatch);

        if (instance)
            instance.destroy();

        logger->info("Cleaned up Vulkan instance manager");

        available_devices.clear();
        messenger = nullptr;
        instance  = nullptr;
    }
} // namespace engine
