#include "backend/vulkan_backend.hpp"

#include "backend/descriptor_pool.hpp"
#include "backend/device_manager.hpp"
#include "backend/instance_manager.hpp"
#include "backend/vertex_description.hpp"
#include "constants.hpp"
#include "exceptions.hpp"
#include "logger.hpp"
#include "window.hpp"
#include <algorithm>
#include <array>
#include <fmt/format.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <limits>
#include <optional>
#include <span>
#include <sstream>
#include <vector>

#include "drawables/GouraudMesh.hpp"

#include "shaders.hpp"

using std::swap, std::string_view, std::vector, std::shared_ptr, std::stringstream, std::span, std::tuple,
    std::numeric_limits, std::clamp, std::min, std::array, std::optional, std::nullopt, std::array;

static vk::SurfaceKHR create_surface(vk::Instance instance, GLFWwindow *window)
{
    VkSurfaceKHR surface = nullptr;

    if (VkResult result = glfwCreateWindowSurface(instance, window, nullptr, (VkSurfaceKHR *)&surface))
        throw engine::VulkanException(result, "Failed to create a GLFW window surface");

    return surface;
}

static std::string_view get_type(vk::PhysicalDeviceType t)
{
    switch (t) {
    case vk::PhysicalDeviceType::eIntegratedGpu:
        return "Integrated";
    case vk::PhysicalDeviceType::eDiscreteGpu:
        return "Discrete";
    case vk::PhysicalDeviceType::eVirtualGpu:
        return "Virtual";
    default:
        return "Other";
    }
}

static void list_devices(shared_ptr<spdlog::logger>                                          logger,
                         span<const tuple<vk::PhysicalDevice, vk::PhysicalDeviceProperties>> devices)
{
    stringstream ss = {};

    for (auto &[device, properties] : devices)
        ss << fmt::format("\n\t[{}] {} ({} MB)", get_type(properties.deviceType), properties.deviceName.data(),
                          properties.limits.maxImageDimension2D);

    logger->info("Found {} supported devices:{}", devices.size(), ss.str());
}

static vector<tuple<vk::PhysicalDevice, vk::PhysicalDeviceProperties>> get_properties(
    span<const vk::PhysicalDevice> devices)
{
    vector<tuple<vk::PhysicalDevice, vk::PhysicalDeviceProperties>> pairs = {};
    pairs.reserve(devices.size());

    for (vk::PhysicalDevice device : devices)
        pairs.push_back({device, device.getProperties()});

    return pairs;
}

static vk::PhysicalDevice select_physical_device(
    span<const tuple<vk::PhysicalDevice, vk::PhysicalDeviceProperties>> devices, vk::SurfaceKHR surface)
{
    if (devices.size() == 0)
        throw engine::Exception("No devices are available");

    vk::PhysicalDevice selected_device;
    uint64_t           selected_score = 0;

    for (auto &[device, properties] : devices) {
        uint64_t score   = 0;
        auto     support = engine::SwapchainSupportDetails::query(device, surface);

        if (support.formats.size() == 0 && support.modes.size() == 0)
            continue;

        score += properties.limits.maxImageDimension2D;
        score += (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) ? 100000 : 0;

        if (score > selected_score)
            selected_device = device, selected_score = score;
    }

    return selected_device;
}

struct ColorBlending
{
    using Attachment = vk::PipelineColorBlendAttachmentState;

    bool                    logic_op_enabled = false;
    vk::LogicOp             logic_op         = {};
    std::vector<Attachment> attachments      = {};
    std::array<float, 4>    constants        = {};
};

struct PipelineConfiguration
{
    using VertexBindingDescriptions   = std::vector<vk::VertexInputBindingDescription>;
    using VertexAttributeDescriptions = std::vector<vk::VertexInputAttributeDescription>;
    using DynamicStates               = std::vector<vk::DynamicState>;
    using RasterizerConfiguration     = vk::PipelineRasterizationStateCreateInfo;
    using MultisampleConfiguration    = vk::PipelineMultisampleStateCreateInfo;

    vk::ShaderModule            vertex_shader                 = {};
    vk::ShaderModule            fragment_shader               = {};
    VertexBindingDescriptions   vertex_binding_descriptions   = {};
    VertexAttributeDescriptions vertex_attribute_descriptions = {};
    DynamicStates               dynamic_states                = {};
    RasterizerConfiguration     rasterizer                    = {};
    MultisampleConfiguration    multisampling                 = {};
    ColorBlending               color_blending                = {};
};

struct PreparedPipelineConfiguration
{
    PreparedPipelineConfiguration(PipelineConfiguration &pipeline_configuration)
        : rasterizer(pipeline_configuration.rasterizer)
        , multisampling(pipeline_configuration.multisampling)
    {
        dynamic_state = {
            .dynamicStateCount = (uint32_t)pipeline_configuration.dynamic_states.size(),
            .pDynamicStates    = pipeline_configuration.dynamic_states.data(),
        };

        shader_stages = {
            {
             .stage  = vk::ShaderStageFlagBits::eVertex,
             .module = pipeline_configuration.vertex_shader,
             .pName  = "main",
             },
            {
             .stage  = vk::ShaderStageFlagBits::eFragment,
             .module = pipeline_configuration.fragment_shader,
             .pName  = "main",
             },
        };

        vertex_input = {
            .vertexBindingDescriptionCount   = (uint32_t)pipeline_configuration.vertex_binding_descriptions.size(),
            .pVertexBindingDescriptions      = pipeline_configuration.vertex_binding_descriptions.data(),
            .vertexAttributeDescriptionCount = (uint32_t)pipeline_configuration.vertex_attribute_descriptions.size(),
            .pVertexAttributeDescriptions    = pipeline_configuration.vertex_attribute_descriptions.data(),
        };

        input_assembly = {
            .topology               = vk::PrimitiveTopology::eTriangleList,
            .primitiveRestartEnable = false,
        };

        viewport_state = {
            .viewportCount = 1,
            .scissorCount  = 1,
        };

        depth_stencil = {
            // TODO - later configuration
        };

        color_blending = {
            .logicOpEnable   = pipeline_configuration.color_blending.logic_op_enabled,
            .logicOp         = pipeline_configuration.color_blending.logic_op,
            .attachmentCount = (uint32_t)pipeline_configuration.color_blending.attachments.size(),
            .pAttachments    = pipeline_configuration.color_blending.attachments.data(),
            .blendConstants  = pipeline_configuration.color_blending.constants,
        };
    }

    vk::PipelineDynamicStateCreateInfo        dynamic_state;
    vector<vk::PipelineShaderStageCreateInfo> shader_stages;
    vk::PipelineVertexInputStateCreateInfo    vertex_input;
    vk::PipelineInputAssemblyStateCreateInfo  input_assembly;
    vk::PipelineViewportStateCreateInfo       viewport_state;
    vk::PipelineRasterizationStateCreateInfo  rasterizer;
    vk::PipelineMultisampleStateCreateInfo    multisampling;
    vk::PipelineDepthStencilStateCreateInfo   depth_stencil;
    vk::PipelineColorBlendStateCreateInfo     color_blending;
};

static vk::ShaderModule create_shader_module(vk::Device device, span<const uint32_t> spirv_code)
{
    vk::ShaderModuleCreateInfo create_info = {
        .codeSize = (uint32_t)spirv_code.size_bytes(),
        .pCode    = spirv_code.data(),
    };

    return device.createShaderModule(create_info);
}

namespace engine
{
    static vk::SurfaceFormatKHR select_format(span<const vk::SurfaceFormatKHR> formats)
    {
        for (auto &format : formats)
            if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
                return format;

        return formats[0];
    }

    static vk::PresentModeKHR select_present_mode(span<const vk::PresentModeKHR> modes)
    {
        for (auto mode : modes)
            if (mode == vk::PresentModeKHR::eMailbox)
                return mode;

        return vk::PresentModeKHR::eFifo;
    }

    static vk::Extent2D select_extent(const vk::SurfaceCapabilitiesKHR &capabilities, GLFWwindow *window)
    {
        if (capabilities.currentExtent.width != numeric_limits<uint32_t>::max())
            return capabilities.currentExtent;
        else {
            uint32_t width = 0, height = 0;
            glfwGetFramebufferSize(window, (int *)&width, (int *)&height);

            return vk::Extent2D {
                .width  = clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                .height = clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
            };
        }
    }

    SwapchainSupportDetails SwapchainSupportDetails::query(vk::PhysicalDevice device, vk::SurfaceKHR surface)
    {
        return SwapchainSupportDetails {
            .capabilities = device.getSurfaceCapabilitiesKHR(surface),
            .formats      = device.getSurfaceFormatsKHR(surface),
            .modes        = device.getSurfacePresentModesKHR(surface),
        };
    }

    bool SwapchainSupportDetails::supported(vk::PhysicalDevice device, vk::SurfaceKHR surface)
    {
        uint32_t format_count = 0;
        uint32_t mode_count   = 0;

        if (VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr))
            throw VulkanException((uint32_t)result, "Failed to query surface formats");

        if (VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &mode_count, nullptr))
            throw VulkanException((uint32_t)result, "Failed to query surface present modes");

        return format_count > 0 && mode_count > 0;
    }

    VulkanBackend::VulkanBackend(string_view application_name, Version application_version, GLFWwindow *window)
        : m_logger(get_logger())
        , m_window(window)
    {
        m_instance_manager = VulkanInstanceManager::new_shared(application_name, application_version);

        auto devices = get_properties(
            m_instance_manager->get_supported_rendering_devices(RenderDeviceManager::get_required_device_extensions(),
                                                                RenderDeviceManager::get_required_device_features()));
        list_devices(m_instance_manager->logger, devices);

        m_surface = create_surface(m_instance_manager->instance, window);
        m_logger->info("Created surface");

        auto device = select_physical_device(devices, m_surface);
        if (device == nullptr)
            throw Exception("No supported devices available");

        m_device_manager = RenderDeviceManager::new_shared(m_instance_manager, device);
        m_device         = m_device_manager->device;
        m_graphics_queue = m_device_manager->graphics_queue.handle;
        m_present_queue  = m_device_manager->present_queue.handle;

        glfwSetFramebufferSizeCallback(m_window, handle_framebuffer_resize);

        create_pipeline();
    }

    VulkanBackend::VulkanBackend(const VulkanBackend &other, GLFWwindow *window)
        : m_logger(other.m_logger)
        , m_window(window)
        , m_instance_manager(other.m_instance_manager)
        , m_device_manager(other.m_device_manager)
        , m_device(other.m_device)
        , m_graphics_queue(other.m_graphics_queue)
        , m_present_queue(other.m_present_queue)
    {
        m_surface = create_surface(m_instance_manager->instance, window);
        m_logger->info("Created surface");

        if (!SwapchainSupportDetails::supported(m_device_manager->physical_device, m_surface))
            throw Exception("The device passed does not support this surface");

        glfwSetFramebufferSizeCallback(m_window, handle_framebuffer_resize);

        create_pipeline();
    }

    VulkanBackend::~VulkanBackend()
    {
        destroy_swapchain();

        m_staging_buffer.deinit(m_device, m_command_pool.get_pool(), *m_allocator);

        m_descriptor_pool.destroy();

        for (auto &frame_set : m_frame_sets)
            frame_set.sync.destroy(m_device);

        if (m_gouraud_pipeline)
            m_device.destroyPipeline(m_gouraud_pipeline);

        if (m_textured_pipeline)
            m_device.destroyPipeline(m_textured_pipeline);

        m_command_pool.destroy();

        if (m_pipeline_layout)
            m_device.destroyPipelineLayout(m_pipeline_layout);

        if (m_uniform_descriptor_layout)
            m_device.destroyDescriptorSetLayout(m_uniform_descriptor_layout);

        if (m_render_pass)
            m_device.destroyRenderPass(m_render_pass);

        if (m_vertex_shader)
            m_device.destroyShaderModule(m_vertex_shader);
        if (m_fragment_shader)
            m_device.destroyShaderModule(m_fragment_shader);

        if (m_swapchain)
            m_device.destroySwapchainKHR(m_swapchain);

        if (m_surface)
            m_instance_manager->instance.destroySurfaceKHR(m_surface);

        m_logger->info("Destroyed render manager");

        m_frame_sets.clear();
        m_allocator         = nullptr;
        m_gouraud_pipeline  = nullptr;
        m_textured_pipeline = nullptr;
        m_pipeline_layout   = nullptr;
        m_render_pass       = nullptr;
        m_swapchain         = nullptr;
        m_surface           = nullptr;
        m_window            = nullptr;
        m_device            = nullptr;
        m_graphics_queue    = nullptr;
        m_present_queue     = nullptr;
        m_device_manager    = nullptr;
        m_instance_manager  = nullptr;
    }

    VulkanBackend::Shared VulkanBackend::new_shared(string_view application_name, Version application_version,
                                                    GLFWwindow *window)
    {
        return Shared(new VulkanBackend(application_name, application_version, window));
    }

    VulkanBackend::Shared VulkanBackend::new_shared(const Shared &other, GLFWwindow *window)
    {
        return Shared(new VulkanBackend(*other, window));
    }

    void VulkanBackend::wait_idle()
    {
        m_device.waitIdle();
    }

    VulkanBackend::VulkanBackend(VulkanBackend &&other) noexcept
        : m_logger(std::move(other.m_logger))
        , m_instance_manager(std::move(other.m_instance_manager))
        , m_device_manager(std::move(other.m_device_manager))
        , m_window(std::move(other.m_window))
        , m_device(std::move(other.m_device))
        , m_graphics_queue(std::move(other.m_graphics_queue))
        , m_present_queue(std::move(other.m_present_queue))
        , m_surface(std::move(other.m_surface))
        , m_images(std::move(other.m_images))
        , m_image_views(std::move(other.m_image_views))
        , m_render_pass(std::move(other.m_render_pass))
        , m_pipeline_layout(std::move(other.m_pipeline_layout))
        , m_gouraud_pipeline(std::move(other.m_gouraud_pipeline))
        , m_command_pool(std::move(other.m_command_pool))
        , m_frame_sets(std::move(other.m_frame_sets))
        , m_descriptor_pool(std::move(other.m_descriptor_pool))
        , m_swapchain_config(std::move(other.m_swapchain_config))
        , m_vertex_shader(std::move(other.m_vertex_shader))
        , m_fragment_shader(std::move(other.m_fragment_shader))
        , m_allocator(std::move(other.m_allocator))
        , m_staging_buffer(std::move(other.m_staging_buffer))
    { }

    VulkanBackend &VulkanBackend::operator=(VulkanBackend &&other) noexcept
    {
        swap(m_logger, other.m_logger);
        swap(m_instance_manager, other.m_instance_manager);
        swap(m_device_manager, other.m_device_manager);
        swap(m_window, other.m_window);
        swap(m_device, other.m_device);
        swap(m_graphics_queue, other.m_graphics_queue);
        swap(m_present_queue, other.m_present_queue);

        swap(m_surface, other.m_surface);
        swap(m_swapchain, other.m_swapchain);
        swap(m_images, other.m_images);
        swap(m_image_views, other.m_image_views);
        swap(m_render_pass, other.m_render_pass);
        swap(m_pipeline_layout, other.m_pipeline_layout);
        swap(m_gouraud_pipeline, other.m_gouraud_pipeline);
        swap(m_command_pool, other.m_command_pool);
        swap(m_frame_sets, other.m_frame_sets);
        swap(m_descriptor_pool, other.m_descriptor_pool);

        swap(m_allocator, other.m_allocator);
        swap(m_staging_buffer, other.m_staging_buffer);

        swap(m_swapchain_config, other.m_swapchain_config);
        swap(m_vertex_shader, other.m_vertex_shader);
        swap(m_fragment_shader, other.m_fragment_shader);

        return *this;
    }

    void VulkanBackend::handle_framebuffer_resize(GLFWwindow *window, int width, int height)
    {
        Window *w = (Window *)glfwGetWindowUserPointer(window);

        w->m_render_manager->m_framebuffer_resized = true;
    }

    void VulkanBackend::create_render_pass()
    {
        vk::AttachmentDescription color_attachment = {
            .format         = m_swapchain_config.format,
            .samples        = vk::SampleCountFlagBits::e1,
            .loadOp         = vk::AttachmentLoadOp::eClear,
            .storeOp        = vk::AttachmentStoreOp::eStore,
            .stencilLoadOp  = vk::AttachmentLoadOp::eDontCare,
            .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
            .initialLayout  = vk::ImageLayout::eUndefined,
            .finalLayout    = vk::ImageLayout::ePresentSrcKHR,
        };

        vk::AttachmentReference color_attachment_reference = {
            .attachment = 0,
            .layout     = vk::ImageLayout::eColorAttachmentOptimal,
        };

        vk::SubpassDescription subpass_description = {
            .pipelineBindPoint    = vk::PipelineBindPoint::eGraphics,
            .colorAttachmentCount = 1,
            .pColorAttachments    = &color_attachment_reference,
        };

        vk::SubpassDependency subpass = {
            .srcSubpass    = vk::SubpassExternal,
            .dstSubpass    = 0,
            .srcStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput,
            .dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput,
            .srcAccessMask = vk::AccessFlagBits::eNone,
            .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
        };

        vk::RenderPassCreateInfo render_pass_create_info = {
            .attachmentCount = 1,
            .pAttachments    = &color_attachment,
            .subpassCount    = 1,
            .pSubpasses      = &subpass_description,
            .dependencyCount = 1,
            .pDependencies   = &subpass,
        };

        m_render_pass = m_device.createRenderPass(render_pass_create_info);
        m_logger->info("Created render pass");
    }

    void VulkanBackend::create_pipeline()
    {
        create_swapchain();
        get_images();
        create_render_pass();
        load_shaders();
        create_descriptor_set_layout();
        create_render_pipeline();
        create_framebuffers();
        create_command_pool();
        create_descriptor_pools();
        initialize_frame_sets();
        initialize_device_memory_allocator();

        finalize_init();

        m_valid_framebuffer = m_swapchain_config.extent.width != 0 && m_swapchain_config.extent.height != 0;
    }

    void VulkanBackend::destroy_swapchain()
    {
        wait_idle();

        for (vk::Framebuffer framebuffer : m_framebuffers)
            m_device.destroyFramebuffer(framebuffer);
        m_framebuffers.clear();

        for (vk::ImageView view : m_image_views)
            m_device.destroyImageView(view);

        m_image_views.clear();
        m_images.clear();

        m_logger->info("Destroyed swapchain");
    }

    bool VulkanBackend::recreate_swapchain()
    {
        if (m_valid_framebuffer)
            destroy_swapchain();

        int width = 0, height = 0;
        glfwGetFramebufferSize(m_window, &width, &height);

        if (width == 0 || height == 0) {
            return false;
        }

        create_swapchain();
        get_images();
        create_framebuffers();

        return true;
    }

    void VulkanBackend::create_swapchain()
    {
        auto swapchain_support_details = SwapchainSupportDetails::query(m_device_manager->physical_device, m_surface);

        auto format                     = select_format(swapchain_support_details.formats);
        m_swapchain_config.format       = format.format;
        m_swapchain_config.color_space  = format.colorSpace;
        m_swapchain_config.present_mode = select_present_mode(swapchain_support_details.modes);
        m_swapchain_config.extent       = select_extent(swapchain_support_details.capabilities, m_window);

        uint32_t image_count = swapchain_support_details.capabilities.minImageCount + 1;
        if (auto max = swapchain_support_details.capabilities.maxImageCount)
            image_count = min(image_count, max);
        m_swapchain_config.image_count = image_count;

        m_swapchain_config.image_layers = 1;

        uint32_t queue_families[2] = {m_device_manager->graphics_queue.index, m_device_manager->present_queue.index};

        bool same_queues = queue_families[0] == queue_families[1];

        vk::SwapchainCreateInfoKHR swapchain_create_info = {
            .surface               = m_surface,
            .minImageCount         = m_swapchain_config.image_count,
            .imageFormat           = m_swapchain_config.format,
            .imageColorSpace       = m_swapchain_config.color_space,
            .imageExtent           = m_swapchain_config.extent,
            .imageArrayLayers      = m_swapchain_config.image_layers,
            .imageUsage            = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode      = same_queues ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
            .queueFamilyIndexCount = same_queues ? 0u : 2u,
            .pQueueFamilyIndices   = same_queues ? nullptr : queue_families,
            .preTransform          = swapchain_support_details.capabilities.currentTransform,
            .compositeAlpha        = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode           = m_swapchain_config.present_mode,
            .clipped               = true,
            .oldSwapchain          = m_swapchain, // Should be NULL for the first call
        };

        m_swapchain = m_device.createSwapchainKHR(swapchain_create_info);
        m_logger->info("Created swapchain");
    }

    void VulkanBackend::load_shaders()
    {
        m_vertex_shader   = create_shader_module(m_device, vertex_shader);
        m_fragment_shader = create_shader_module(m_device, fragment_shader);
    }

    void VulkanBackend::create_descriptor_set_layout()
    {
        constexpr vk::DescriptorSetLayoutBinding vp_layout = {
            .binding            = 0,
            .descriptorType     = vk::DescriptorType::eUniformBuffer,
            .descriptorCount    = 1,
            .stageFlags         = vk::ShaderStageFlagBits::eVertex,
            .pImmutableSamplers = nullptr,
        };

        constexpr vk::DescriptorSetLayoutBinding model_layout = {
            .binding            = 1,
            .descriptorType     = vk::DescriptorType::eUniformBuffer,
            .descriptorCount    = 1,
            .stageFlags         = vk::ShaderStageFlagBits::eVertex,
            .pImmutableSamplers = nullptr,
        };

        std::array<vk::DescriptorSetLayoutBinding, 2> descriptor_sets = {vp_layout, model_layout};

        vk::DescriptorSetLayoutCreateInfo create_info = {
            .bindingCount = descriptor_sets.size(),
            .pBindings    = descriptor_sets.data(),
        };

        m_uniform_descriptor_layout = m_device.createDescriptorSetLayout(create_info);
    }

    void VulkanBackend::create_descriptor_pools()
    {
        m_descriptor_pool.init(m_device_manager, MAX_IN_FLIGHT * MAX_DESCRIPTORS);
    }

    static vector<vk::ImageView> create_image_views(vk::Device device, span<vk::Image> images, vk::Format format,
                                                    uint32_t image_layers)
    {
        vector<vk::ImageView> image_views;
        image_views.reserve(images.size());

        for (vk::Image image : images) {
            vk::ImageViewCreateInfo create_info = {
                .image            = image,
                .viewType         = vk::ImageViewType::e2D,
                .format           = format,
                .components       = {.r = vk::ComponentSwizzle::eIdentity,
                                     .g = vk::ComponentSwizzle::eIdentity,
                                     .b = vk::ComponentSwizzle::eIdentity,
                                     .a = vk::ComponentSwizzle::eIdentity},
                .subresourceRange = {.aspectMask     = vk::ImageAspectFlagBits::eColor,
                                     .baseMipLevel   = 0,
                                     .levelCount     = 1,
                                     .baseArrayLayer = 0,
                                     .layerCount     = image_layers},
            };

            image_views.push_back(device.createImageView(create_info));
        }

        return image_views;
    }

    void VulkanBackend::get_images()
    {
        m_images = m_device.getSwapchainImagesKHR(m_swapchain);

        m_image_views =
            create_image_views(m_device, m_images, m_swapchain_config.format, m_swapchain_config.image_layers);

        m_logger->info("Created {} image views", m_image_views.size());
    }

    void VulkanBackend::create_render_pipeline()
    {
        vk::PipelineLayoutCreateInfo pipeline_layout_create_info = {
            .setLayoutCount         = 1,
            .pSetLayouts            = &m_uniform_descriptor_layout,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges    = nullptr,
        };

        m_pipeline_layout = m_device.createPipelineLayout(pipeline_layout_create_info);
        m_logger->info("Created render pipeline layout");

        PipelineConfiguration pipeline_config = {};

        pipeline_config.vertex_shader   = m_vertex_shader;
        pipeline_config.fragment_shader = m_fragment_shader;
        m_logger->info("Loaded default vertex and fragment shaders");

        pipeline_config.dynamic_states = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor,
        };

        pipeline_config.rasterizer = {
            .depthClampEnable        = false,
            .rasterizerDiscardEnable = false,
            .polygonMode             = vk::PolygonMode::eFill,
            .cullMode                = vk::CullModeFlagBits::eBack,
            .frontFace               = vk::FrontFace::eClockwise,
            .depthBiasEnable         = false,
            .depthBiasConstantFactor = 0.0,
            .depthBiasClamp          = 0.0,
            .depthBiasSlopeFactor    = 0.0,
            .lineWidth               = 1.0f,
        };

        pipeline_config.multisampling = {
            .rasterizationSamples  = vk::SampleCountFlagBits::e1,
            .sampleShadingEnable   = false,
            .minSampleShading      = 1.0,
            .pSampleMask           = nullptr,
            .alphaToCoverageEnable = false,
            .alphaToOneEnable      = false,
        };

        using ColorComponent           = vk::ColorComponentFlagBits;
        pipeline_config.color_blending = {
            .logic_op_enabled = false,
            .logic_op         = vk::LogicOp::eCopy,
            .attachments      = {{
                     .blendEnable         = false,
                     .srcColorBlendFactor = vk::BlendFactor::eOne,
                     .dstColorBlendFactor = vk::BlendFactor::eOne,
                     .colorBlendOp        = vk::BlendOp::eAdd,
                     .srcAlphaBlendFactor = vk::BlendFactor::eOne,
                     .dstAlphaBlendFactor = vk::BlendFactor::eZero,
                     .alphaBlendOp        = vk::BlendOp::eAdd,
                     .colorWriteMask = ColorComponent::eR | ColorComponent::eG | ColorComponent::eB | ColorComponent::eA,
            }},
            .constants        = {0.0, 0.0, 0.0, 0.0},
        };

        using primitives::GOURAUD_VERTEX;

        pipeline_config.vertex_binding_descriptions =
            vector(GOURAUD_VERTEX.bindings.begin(), GOURAUD_VERTEX.bindings.end());
        pipeline_config.vertex_attribute_descriptions =
            vector(GOURAUD_VERTEX.attributes.begin(), GOURAUD_VERTEX.attributes.end());

        auto config = PreparedPipelineConfiguration(pipeline_config);

        {
            vk::GraphicsPipelineCreateInfo gouraud_pipeline_info = {
                .stageCount          = (uint32_t)config.shader_stages.size(),
                .pStages             = config.shader_stages.data(),
                .pVertexInputState   = &config.vertex_input,
                .pInputAssemblyState = &config.input_assembly,
                .pViewportState      = &config.viewport_state,
                .pRasterizationState = &config.rasterizer,
                .pMultisampleState   = &config.multisampling,
                .pDepthStencilState  = nullptr,
                .pColorBlendState    = &config.color_blending,
                .pDynamicState       = &config.dynamic_state,
                .layout              = m_pipeline_layout,
                .renderPass          = m_render_pass,
                .subpass             = 0,
                .basePipelineHandle  = nullptr,
                .basePipelineIndex   = -1,
            };

            auto [result, pipeline] = m_device.createGraphicsPipeline(nullptr, gouraud_pipeline_info);
            if (result != vk::Result::eSuccess)
                throw VulkanException((uint32_t)result, "Failed to create graphics pipeline");
            m_logger->info("Created graphics pipeline");

            m_gouraud_pipeline = pipeline;
        }

        using primitives::TEXTURED_VERTEX;
        pipeline_config.vertex_binding_descriptions =
            vector(TEXTURED_VERTEX.bindings.begin(), TEXTURED_VERTEX.bindings.end());
        pipeline_config.vertex_attribute_descriptions =
            vector(TEXTURED_VERTEX.attributes.begin(), TEXTURED_VERTEX.attributes.end());

        config = pipeline_config;

        {
            vk::GraphicsPipelineCreateInfo textured_pipeline_info = {
                .stageCount          = (uint32_t)config.shader_stages.size(),
                .pStages             = config.shader_stages.data(),
                .pVertexInputState   = &config.vertex_input,
                .pInputAssemblyState = &config.input_assembly,
                .pViewportState      = &config.viewport_state,
                .pRasterizationState = &config.rasterizer,
                .pMultisampleState   = &config.multisampling,
                .pDepthStencilState  = nullptr,
                .pColorBlendState    = &config.color_blending,
                .pDynamicState       = &config.dynamic_state,
                .layout              = m_pipeline_layout,
                .renderPass          = m_render_pass,
                .subpass             = 0,
                .basePipelineHandle  = nullptr,
                .basePipelineIndex   = -1,
            };

            auto [result, pipeline] = m_device.createGraphicsPipeline(nullptr, textured_pipeline_info);
            if (result != vk::Result::eSuccess)
                throw VulkanException((uint32_t)result, "Failed to create graphics pipeline");
            m_logger->info("Created graphics pipeline");

            m_textured_pipeline = pipeline;
        }
    }

    void VulkanBackend::create_framebuffers()
    {
        m_framebuffers.resize(m_images.size());

        for (size_t i = 0; i < m_framebuffers.size(); ++i) {
            array<vk::ImageView, 1> attachments = {
                m_image_views[i],
            };

            vk::FramebufferCreateInfo framebuffer_create_info = {
                .renderPass      = m_render_pass,
                .attachmentCount = attachments.size(),
                .pAttachments    = attachments.data(),
                .width           = m_swapchain_config.extent.width,
                .height          = m_swapchain_config.extent.height,
                .layers          = m_swapchain_config.image_layers,
            };

            m_framebuffers[i] = m_device.createFramebuffer(framebuffer_create_info);
        }

        m_logger->info("Created framebuffers");
    }

    void VulkanBackend::create_command_pool()
    {
        m_command_pool.init(m_device_manager, m_device_manager->graphics_queue.index);
    }

    void VulkanBackend::initialize_frame_sets()
    {
        auto cmd_buffers     = m_command_pool.get(MAX_IN_FLIGHT);
        auto descriptor_sets = m_descriptor_pool.get(m_uniform_descriptor_layout, MAX_IN_FLIGHT * MAX_DESCRIPTORS);

        m_frame_sets.resize(MAX_IN_FLIGHT);
        for (size_t i = 0; i < MAX_IN_FLIGHT; ++i) {
            m_frame_sets[i].command_buffer = cmd_buffers[i];
            m_frame_sets[i].sync.init(m_device);
            memcpy(m_frame_sets[i].descriptors.data(), descriptor_sets.data() + i * MAX_DESCRIPTORS, MAX_DESCRIPTORS);
        }
    }

    void VulkanBackend::initialize_device_memory_allocator()
    {
        m_allocator           = shared_ptr<VulkanAllocator>(new VulkanAllocator(m_device_manager));
        vk::CommandBuffer cmd = m_command_pool.get();
        vk::Fence fence       = m_device.createFence(vk::FenceCreateInfo {.flags = vk::FenceCreateFlagBits::eSignaled});

        m_staging_buffer.init(*m_allocator, cmd, fence);
    }

    void VulkanBackend::finalize_init()
    {
        m_vp_uniform = TypedHostVisibleAllocation<ViewProjectionUniform[MAX_IN_FLIGHT]>(
            m_allocator, vk::BufferUsageFlagBits::eUniformBuffer);
    }

    optional<DrawingContext> VulkanBackend::begin_draw()
    {
        constexpr uint64_t TIMEOUT = std::numeric_limits<uint64_t>::max();

        if (!m_valid_framebuffer) {
            m_valid_framebuffer = recreate_swapchain();
            return nullopt;
        }

        uint32_t  frame = m_frame_index;
        FrameSet &set   = m_frame_sets[frame];

        auto wait_result = m_device.waitForFences(set.sync.in_flight, true, TIMEOUT);
        if (wait_result != vk::Result::eSuccess)
            throw VulkanException((uint32_t)wait_result, "Failed to wait on fence");

        auto [ia_result, image_index] = m_device.acquireNextImageKHR(m_swapchain, TIMEOUT, set.sync.image_available);
        if (ia_result == vk::Result::eErrorOutOfDateKHR) {
            m_valid_framebuffer = recreate_swapchain();
            return nullopt;
        } else if (ia_result != vk::Result::eSuccess)
            throw VulkanException((uint32_t)ia_result, "Failed to acquire image");

        m_device.resetFences(set.sync.in_flight);

        set.command_buffer.reset();
        initialize_command_buffer(set.command_buffer, image_index);

        m_vp_uniform[frame] = {
            .view       = m_camera,
            .projection = glm::perspectiveFovZO<float>(glm::radians(m_fov), m_swapchain_config.extent.width,
                                                       m_swapchain_config.extent.height, 0.1, 10.0),
        };
        m_vp_uniform[frame].projection[1][1] *= -1.0f;
        m_vp_uniform.flush();

        vk::DescriptorBufferInfo dbi = {
            .buffer = m_vp_uniform.buffer,
            .offset = m_vp_uniform.offset(frame),
            .range  = m_vp_uniform.type_size(),
        };

        return DrawingContext {
            .backend               = this,
            .descriptors           = set.descriptors,
            .used_descriptors      = 0,
            .frame_index           = frame,
            .swapchain_image_index = image_index,
            .vp_buffer_info        = dbi,
            .cmd                   = set.command_buffer,
        };
    }

    void VulkanBackend::end_draw(DrawingContext &context)
    {
        size_t    frame_index = context.frame_index;
        uint32_t  image_index = context.swapchain_image_index;
        FrameSet &set         = m_frame_sets[frame_index];

        set.command_buffer.endRenderPass();
        set.command_buffer.end();

        vk::PipelineStageFlags wait_stages = vk::PipelineStageFlagBits::eColorAttachmentOutput;

        vk::SubmitInfo submit = {
            .waitSemaphoreCount   = 1,
            .pWaitSemaphores      = &set.sync.image_available,
            .pWaitDstStageMask    = &wait_stages,
            .commandBufferCount   = 1,
            .pCommandBuffers      = &set.command_buffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores    = &set.sync.render_finished,
        };

        m_graphics_queue.submit(submit, set.sync.in_flight);

        vk::PresentInfoKHR present = {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores    = &set.sync.render_finished,
            .swapchainCount     = 1,
            .pSwapchains        = &m_swapchain,
            .pImageIndices      = &image_index,
            .pResults           = nullptr,
        };

        bool should_recreate_swapchain = false;
        try {
            should_recreate_swapchain = m_present_queue.presentKHR(present) == vk::Result::eSuboptimalKHR;
        } catch (vk::OutOfDateKHRError) {
            should_recreate_swapchain = true;
        }

        if (should_recreate_swapchain || m_framebuffer_resized) {
            m_framebuffer_resized = false;
            m_valid_framebuffer   = recreate_swapchain();
        }

        m_frame_index = ++m_frame_index % MAX_IN_FLIGHT;
    }

    void VulkanBackend::initialize_command_buffer(vk::CommandBuffer buffer, uint32_t image_index)
    {
        vk::CommandBufferBeginInfo buffer_begin = {
            .flags            = {},
            .pInheritanceInfo = nullptr,
        };

        buffer.begin(buffer_begin);

        vk::ClearValue clear_value = vk::ClearValue {vk::ClearColorValue {array {0.0f, 0.0f, 0.0f, 1.0f}}};

        vk::RenderPassBeginInfo render_pass_begin = {
            .renderPass      = m_render_pass,
            .framebuffer     = m_framebuffers[image_index],
            .renderArea      = {.offset = {0, 0}, .extent = m_swapchain_config.extent},
            .clearValueCount = 1,
            .pClearValues    = &clear_value,
        };

        vk::Viewport viewport = {
            .x        = 0.0,
            .y        = 0.0,
            .width    = (float)m_swapchain_config.extent.width,
            .height   = (float)m_swapchain_config.extent.height,
            .minDepth = 0.0,
            .maxDepth = 1.0,
        };

        vk::Rect2D scissor = {
            .offset = {0, 0},
            .extent = m_swapchain_config.extent,
        };

        buffer.beginRenderPass(render_pass_begin, vk::SubpassContents::eInline);
        buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_gouraud_pipeline);
        buffer.setViewport(0, viewport);
        buffer.setScissor(0, scissor);
    }

    void GpuSync::init(vk::Device device)
    {
        vk::SemaphoreCreateInfo sem = {};
        vk::FenceCreateInfo     fen = {
                .flags = vk::FenceCreateFlagBits::eSignaled,
        };

        image_available = device.createSemaphore(sem);

        render_finished = device.createSemaphore(sem);

        in_flight = device.createFence(fen);
    }

    void GpuSync::destroy(vk::Device device)
    {
        if (image_available)
            device.destroySemaphore(image_available);

        if (render_finished)
            device.destroySemaphore(render_finished);

        if (in_flight)
            device.destroyFence(in_flight);

        image_available = nullptr;
        render_finished = nullptr;
        in_flight       = nullptr;
    }

    void VulkanBackend::update_projection(float fov)
    {
        m_fov = fov;
    }

    void VulkanBackend::set_view(const glm::mat4 &transformation)
    {
        m_camera = transformation;
    }

    shared_ptr<GouraudMesh> VulkanBackend::load(span<primitives::GouraudVertex> vertices, span<uint32_t> indices)
    {
        constexpr vk::BufferUsageFlags BUFFER_USAGE = vk::BufferUsageFlagBits::eVertexBuffer
                                                    | vk::BufferUsageFlagBits::eIndexBuffer
                                                    | vk::BufferUsageFlagBits::eTransferDst;

        size_t vbuf_bytes  = vertices.size_bytes();   // Vertex buffer bytes
        size_t ibuf_bytes  = indices.size_bytes();    // Index buffer bytes
        size_t total_bytes = vbuf_bytes + ibuf_bytes; // Total bytes

        Allocation allocation(m_allocator, total_bytes, BUFFER_USAGE);

        // Transferring must be performed such that the writing buffer does not overflow.

        size_t buffer_off = 0; // Buffer offset
        size_t rdbuff_off = 0; // Reading buffer offset

        // Transfer vertex buffer
        while (rdbuff_off < vbuf_bytes) {
            m_staging_buffer.wait(m_device);
            m_staging_buffer.reset(m_device); // Lock fence

            size_t bytes = std::min(vbuf_bytes - rdbuff_off, (size_t)StagingBuffer::SIZE);
            memcpy(m_staging_buffer, vertices.data() + rdbuff_off, bytes);
            rdbuff_off += bytes;

            m_staging_buffer.flush(*m_allocator, 0, bytes);
            m_staging_buffer.transfer(allocation.buffer, m_graphics_queue, 0, buffer_off, bytes);
            buffer_off += bytes;
        }

        rdbuff_off = 0;

        // Transfer index buffer
        while (rdbuff_off < ibuf_bytes) {
            m_staging_buffer.wait(m_device);
            m_staging_buffer.reset(m_device); // Lock fence

            size_t bytes = std::min(ibuf_bytes - rdbuff_off, (size_t)StagingBuffer::SIZE);
            memcpy(m_staging_buffer, indices.data() + rdbuff_off, bytes);
            rdbuff_off += bytes;

            m_staging_buffer.flush(*m_allocator, 0, bytes);
            m_staging_buffer.transfer(allocation.buffer, m_graphics_queue, 0, buffer_off, bytes);
            buffer_off += bytes;
        }

        m_staging_buffer.wait(m_device); // Wait for final transfer

        return shared_ptr<GouraudMesh>(new GouraudMesh(std::move(allocation), 0, vbuf_bytes, indices.size()));
    }

    VulkanBackend::StagingBuffer::operator uint8_t *()
    {
        return (uint8_t *)p_mapping;
    }

    void VulkanBackend::StagingBuffer::init(VmaAllocator allocator, vk::CommandBuffer cmd, vk::Fence fence)
    {
        vk::BufferCreateInfo bufc = {
            .size  = SIZE,
            .usage = vk::BufferUsageFlagBits::eTransferSrc,
        };

        VmaAllocationCreateInfo vma_alloc = {
            .flags          = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage          = VMA_MEMORY_USAGE_AUTO,
            .preferredFlags = VkMemoryPropertyFlags(vk::MemoryPropertyFlagBits::eHostCoherent),
        };

        VmaAllocationInfo alloc_info;

        if (vmaCreateBuffer(allocator, (VkBufferCreateInfo *)&bufc, &vma_alloc, (VkBuffer *)&buffer, &alloc,
                            &alloc_info)) {
            vma_alloc.preferredFlags = 0;
            if (VkResult result = vmaCreateBuffer(allocator, (VkBufferCreateInfo *)&bufc, &vma_alloc,
                                                  (VkBuffer *)&buffer, &alloc, &alloc_info))
                throw VulkanException(result, "Failed to create staging buffer");

            is_coherent = false;
        } else
            is_coherent = true;

        p_mapping      = alloc_info.pMappedData;
        this->cmd      = cmd;
        transfer_fence = fence;
    }

    void VulkanBackend::StagingBuffer::flush(VmaAllocator allocator, vk::DeviceSize offset, vk::DeviceSize length)
    {
        if (!is_coherent)
            vmaFlushAllocation(allocator, alloc, offset, length);
    }

    void VulkanBackend::StagingBuffer::transfer(vk::Buffer dst, vk::Queue queue, uint32_t src_offset,
                                                uint32_t dst_offset, uint32_t size)
    {
        cmd.reset();
        cmd.begin(vk::CommandBufferBeginInfo {});
        cmd.copyBuffer(buffer, dst, vk::BufferCopy {.srcOffset = src_offset, .dstOffset = dst_offset, .size = size});
        cmd.end();

        queue.submit(vk::SubmitInfo {.commandBufferCount = 1, .pCommandBuffers = &cmd}, transfer_fence);
    }

    void VulkanBackend::StagingBuffer::reset(vk::Device device)
    {
        device.resetFences(transfer_fence);
    }

    void VulkanBackend::StagingBuffer::wait(vk::Device device)
    {
        vk::Result result = device.waitForFences(transfer_fence, true, std::numeric_limits<uint64_t>::max());
        if (result != vk::Result::eSuccess)
            throw VulkanException((uint32_t)result, "Failed to wait on transfer fence");
    }

    void VulkanBackend::StagingBuffer::deinit(vk::Device device, vk::CommandPool cmd_pool, VmaAllocator allocator)
    {
        if (!buffer)
            return;

        device.destroyFence(transfer_fence);
        device.freeCommandBuffers(cmd_pool, cmd);
        vmaDestroyBuffer(allocator, buffer, alloc);
        transfer_fence = nullptr;
        cmd            = nullptr;
        buffer         = nullptr;
        alloc          = nullptr;
        is_coherent    = false;
        p_mapping      = nullptr;
    }
} // namespace engine
