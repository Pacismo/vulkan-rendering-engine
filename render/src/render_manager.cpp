#include "render_manager.hpp"
#include "exceptions.hpp"
#include "logger.hpp"
#include <algorithm>
#include <array>
#include <device_manager.hpp>
#include <fmt/format.h>
#include <instance_manager.hpp>
#include <limits>
#include <span>
#include <sstream>

#include "shaders.hpp"

using std::move, std::swap, std::string_view, std::vector, std::shared_ptr, std::stringstream, std::span, std::tuple,
    std::numeric_limits, std::clamp, std::min, std::array;

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

struct PreparedPipelineConfiguration
{
    PreparedPipelineConfiguration(engine::PipelineConfiguration &pipeline_configuration,
                                  vk::PipelineLayout             pipeline_layout)
        : pipeline_layout(pipeline_layout)
        , rasterizer(pipeline_configuration.rasterizer)
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
    vk::PipelineRasterizationStateCreateInfo &rasterizer;
    vk::PipelineMultisampleStateCreateInfo   &multisampling;
    vk::PipelineDepthStencilStateCreateInfo   depth_stencil;
    vk::PipelineColorBlendStateCreateInfo     color_blending;
    vk::PipelineLayout                        pipeline_layout;
};

static vk::ShaderModule create_shader_module(vk::Device device, span<const uint32_t> spirv_code)
{
    vk::ShaderModuleCreateInfo create_info = {
        .codeSize = (uint32_t)spirv_code.size_bytes(),
        .pCode    = spirv_code.data(),
    };

    auto [result, smodule] = device.createShaderModule(create_info);
    if (result != vk::Result::eSuccess)
        throw engine::VulkanException((uint32_t)result, "Failed to create a shader module");

    return smodule;
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
        auto capabilities = device.getSurfaceCapabilitiesKHR(surface);
        if (capabilities.result != vk::Result::eSuccess)
            throw VulkanException((uint32_t)capabilities.result, "Failed to query surface capabilities");

        auto formats = device.getSurfaceFormatsKHR(surface);
        if (formats.result != vk::Result::eSuccess)
            throw VulkanException((uint32_t)formats.result, "Failed to query surface formats");

        auto modes = device.getSurfacePresentModesKHR(surface);
        if (modes.result != vk::Result::eSuccess)
            throw VulkanException((uint32_t)modes.result, "Failed to query surface present modes");

        return SwapchainSupportDetails {
            .capabilities = capabilities.value,
            .formats      = formats.value,
            .modes        = modes.value,
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

    RenderManager::RenderManager(string_view application_name, Version application_version, GLFWwindow *window)
        : m_logger(get_logger())
        , m_window(window)
    {
        m_instance_manager = VulkanInstanceManager::new_shared(application_name, application_version);

        auto devices = get_properties(m_instance_manager->get_supported_rendering_devices());
        list_devices(m_instance_manager->m_logger, devices);

        m_surface = create_surface(m_instance_manager->m_instance, window);
        m_logger->info("Created surface");

        auto device = select_physical_device(devices, m_surface);
        if (device == nullptr)
            throw Exception("No supported devices available");

        m_device_manager = RenderDeviceManager::new_shared(m_instance_manager, device);
        m_device         = m_device_manager->m_device;
        m_graphics_queue = m_device_manager->m_graphics_queue.handle;
        m_present_queue  = m_device_manager->m_present_queue.handle;

        initialize_pipeline_configuration();

        create_pipeline();
    }

    RenderManager::RenderManager(const RenderManager &other, GLFWwindow *window)
        : m_logger(other.m_logger)
        , m_window(window)
        , m_instance_manager(other.m_instance_manager)
        , m_device_manager(other.m_device_manager)
        , m_device(other.m_device)
        , m_graphics_queue(other.m_graphics_queue)
        , m_present_queue(other.m_present_queue)
    {
        m_surface = create_surface(m_instance_manager->m_instance, window);
        m_logger->info("Created surface");

        if (!SwapchainSupportDetails::supported(m_device_manager->m_physical_device, m_surface))
            throw Exception("The device passed does not support this surface");

        initialize_pipeline_configuration();

        create_pipeline();
    }

    RenderManager::~RenderManager()
    {
        clean_pipeline();

        m_sync.destroy(m_device);

        if (m_command_pool)
            m_device.destroyCommandPool(m_command_pool);

        if (m_pipeline_layout)
            m_device.destroyPipelineLayout(m_pipeline_layout);

        if (m_render_pass)
            m_device.destroyRenderPass(m_render_pass);

        if (vk::ShaderModule module = m_configuration.pipeline.vertex_shader)
            m_device.destroyShaderModule(module);
        if (vk::ShaderModule module = m_configuration.pipeline.fragment_shader)
            m_device.destroyShaderModule(module);

        if (m_swapchain)
            m_device.destroySwapchainKHR(m_swapchain);

        if (m_surface)
            m_instance_manager->m_instance.destroySurfaceKHR(m_surface);

        m_logger->info("Destroyed render manager");

        m_command_buffers.clear();
        m_sync             = {};
        m_command_pool     = nullptr;
        m_pipeline         = nullptr;
        m_pipeline_layout  = nullptr;
        m_render_pass      = nullptr;
        m_swapchain        = nullptr;
        m_surface          = nullptr;
        m_window           = nullptr;
        m_device           = nullptr;
        m_graphics_queue   = nullptr;
        m_present_queue    = nullptr;
        m_device_manager   = nullptr;
        m_instance_manager = nullptr;
    }

    RenderManager::Unique RenderManager::new_unique(string_view application_name, Version application_version,
                                                    GLFWwindow *window)
    {
        return Unique(new RenderManager(application_name, application_version, window));
    }

    RenderManager::Unique RenderManager::new_unique(const Unique &other, GLFWwindow *window)
    {
        return Unique(new RenderManager(*other, window));
    }

    void RenderManager::render_frame()
    {
        constexpr uint64_t TIMEOUT = std::numeric_limits<uint64_t>::max();

        auto wait_result = m_device.waitForFences(m_sync.in_flight, true, TIMEOUT);
        if (wait_result != vk::Result::eSuccess)
            throw VulkanException((uint32_t)wait_result, "Failed to wait on fence");

        auto reset_result = m_device.resetFences(m_sync.in_flight);
        if (reset_result != vk::Result::eSuccess)
            throw VulkanException((uint32_t)reset_result, "Failed to reset fence");

        auto [ia_result, image_index] = m_device.acquireNextImageKHR(m_swapchain, TIMEOUT, m_sync.image_available);
        if (ia_result != vk::Result::eSuccess)
            throw VulkanException((uint32_t)ia_result, "Failed to acquire image");

        m_command_buffers[0].reset();
        record_command_buffer(m_command_buffers[0], image_index);

        vk::PipelineStageFlags wait_stages = vk::PipelineStageFlagBits::eColorAttachmentOutput;

        vk::SubmitInfo submit = {
            .waitSemaphoreCount   = 1,
            .pWaitSemaphores      = &m_sync.image_available,
            .pWaitDstStageMask    = &wait_stages,
            .commandBufferCount   = 1,
            .pCommandBuffers      = &m_command_buffers[0],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores    = &m_sync.render_finished,
        };

        auto s_result = m_graphics_queue.submit(submit, m_sync.in_flight);
        if (s_result != vk::Result::eSuccess)
            throw VulkanException((uint32_t)s_result, "Failed to submit command buffer");

        vk::PresentInfoKHR present = {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores    = &m_sync.render_finished,
            .swapchainCount     = 1,
            .pSwapchains        = &m_swapchain,
            .pImageIndices      = &image_index,
            .pResults           = nullptr,
        };

        auto p_result = m_present_queue.presentKHR(present);
        if (p_result != vk::Result::eSuccess)
            throw VulkanException((uint32_t)p_result, "Failed to present image");
    }

    void RenderManager::wait_idle()
    {
        auto result = m_device.waitIdle();
        if (result != vk::Result::eSuccess)
            throw VulkanException((uint32_t)result, "Failed to wait for device idle");
    }

    RenderManager::RenderManager(RenderManager &&other) noexcept
        : m_logger(move(other.m_logger))
        , m_instance_manager(move(other.m_instance_manager))
        , m_device_manager(move(other.m_device_manager))
        , m_window(move(other.m_window))
        , m_device(move(other.m_device))
        , m_graphics_queue(move(other.m_graphics_queue))
        , m_present_queue(move(other.m_present_queue))
        , m_surface(move(other.m_surface))
        , m_images(move(other.m_images))
        , m_image_views(move(other.m_image_views))
        , m_render_pass(move(other.m_render_pass))
        , m_pipeline_layout(move(other.m_pipeline_layout))
        , m_pipeline(move(other.m_pipeline))
        , m_command_pool(move(other.m_command_pool))
        , m_command_buffers(move(other.m_command_buffers))
        , m_sync(move(other.m_sync))
        , m_configuration(other.m_configuration)
    { }

    RenderManager &RenderManager::operator=(RenderManager &&other) noexcept
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
        swap(m_pipeline, other.m_pipeline);
        swap(m_command_pool, other.m_command_pool);
        swap(m_command_buffers, other.m_command_buffers);
        swap(m_sync, other.m_sync);

        swap(m_configuration, other.m_configuration);

        return *this;
    }

    void RenderManager::initialize_pipeline_configuration()
    {
        m_configuration.pipeline.vertex_shader   = create_shader_module(m_device, vertex_shader);
        m_configuration.pipeline.fragment_shader = create_shader_module(m_device, fragment_shader);
        m_logger->info("Loaded default vertex and fragment shaders");

        m_configuration.pipeline.dynamic_states = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor,
        };

        m_configuration.pipeline.rasterizer = {
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

        m_configuration.pipeline.multisampling = {
            .rasterizationSamples  = vk::SampleCountFlagBits::e1,
            .sampleShadingEnable   = false,
            .minSampleShading      = 1.0,
            .pSampleMask           = nullptr,
            .alphaToCoverageEnable = false,
            .alphaToOneEnable      = false,
        };

        using ColorComponent                    = vk::ColorComponentFlagBits;
        m_configuration.pipeline.color_blending = {
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
    }

    void RenderManager::create_render_pass()
    {
        vk::AttachmentDescription color_attachment = {
            .format         = m_configuration.swapchain.format,
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

        auto [result, render_pass] = m_device.createRenderPass(render_pass_create_info);
        if (result != vk::Result::eSuccess)
            throw VulkanException((uint32_t)result, "Failed to create render pass");

        m_render_pass = render_pass;
    }

    void RenderManager::create_pipeline()
    {
        create_swapchain();
        get_images();
        create_render_pass();
        create_render_pipeline();
        create_framebuffers();
        create_command_pool();
        allocate_command_buffers();
        create_sync_primitives();
    }

    void RenderManager::clean_pipeline()
    {
        wait_idle();

        for (vk::Framebuffer framebuffer : m_framebuffers)
            m_device.destroyFramebuffer(framebuffer);
        m_framebuffers.clear();

        if (m_pipeline)
            m_device.destroyPipeline(m_pipeline);

        for (vk::ImageView view : m_image_views)
            m_device.destroyImageView(view);

        m_image_views.clear();
        m_images.clear();
    }

    void RenderManager::recreate_pipeline()
    {
        clean_pipeline();
        create_pipeline();
    }

    void RenderManager::create_swapchain()
    {
        auto swapchain_support_details = SwapchainSupportDetails::query(m_device_manager->m_physical_device, m_surface);

        auto format                            = select_format(swapchain_support_details.formats);
        m_configuration.swapchain.format       = format.format;
        m_configuration.swapchain.color_space  = format.colorSpace;
        m_configuration.swapchain.present_mode = select_present_mode(swapchain_support_details.modes);
        m_configuration.swapchain.extent       = select_extent(swapchain_support_details.capabilities, m_window);

        uint32_t image_count = swapchain_support_details.capabilities.minImageCount + 1;
        if (auto max = swapchain_support_details.capabilities.maxImageCount)
            image_count = min(image_count, max);
        m_configuration.swapchain.image_count = image_count;

        m_configuration.swapchain.image_layers = 1;

        uint32_t queue_families[2] = {m_device_manager->m_graphics_queue.index,
                                      m_device_manager->m_present_queue.index};

        bool same_queues = queue_families[0] == queue_families[1];

        vk::SwapchainCreateInfoKHR swapchain_create_info = {
            .surface               = m_surface,
            .minImageCount         = m_configuration.swapchain.image_count,
            .imageFormat           = m_configuration.swapchain.format,
            .imageColorSpace       = m_configuration.swapchain.color_space,
            .imageExtent           = m_configuration.swapchain.extent,
            .imageArrayLayers      = m_configuration.swapchain.image_layers,
            .imageUsage            = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode      = same_queues ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
            .queueFamilyIndexCount = same_queues ? 0u : 2u,
            .pQueueFamilyIndices   = same_queues ? nullptr : queue_families,
            .preTransform          = swapchain_support_details.capabilities.currentTransform,
            .compositeAlpha        = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode           = m_configuration.swapchain.present_mode,
            .clipped               = true,
            .oldSwapchain          = m_swapchain, // Should be NULL for the first call
        };

        auto [result, swapchain] = m_device.createSwapchainKHR(swapchain_create_info);
        if (result != vk::Result::eSuccess)
            throw VulkanException((uint32_t)result, "Failed to create Swapchain");
        m_logger->info("Created swapchain");

        m_swapchain = swapchain;
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

            auto [result, view] = device.createImageView(create_info);
            if (result != vk::Result::eSuccess)
                throw VulkanException((uint32_t)result, "Failed to create an image view");
            image_views.push_back(view);
        }

        return image_views;
    }

    void RenderManager::get_images()
    {
        {
            auto [result, images] = m_device.getSwapchainImagesKHR(m_swapchain);
            if (result != vk::Result::eSuccess)
                throw VulkanException((uint32_t)result, "Failed to get Swapchain images");

            m_images = images;
        }

        m_image_views = create_image_views(m_device, m_images, m_configuration.swapchain.format,
                                           m_configuration.swapchain.image_layers);

        m_logger->info("Created {} image views", m_image_views.size());
    }

    void RenderManager::create_render_pipeline()
    {
        vk::PipelineLayoutCreateInfo pipeline_layout_create_info = {
            .setLayoutCount         = 0,
            .pSetLayouts            = nullptr,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges    = nullptr,
        };

        {
            auto [result, pipeline_layout] = m_device.createPipelineLayout(pipeline_layout_create_info);
            if (result != vk::Result::eSuccess)
                throw VulkanException((uint32_t)result, "Failed to create a pipeline layout");
            m_logger->info("Created render pipeline layout");

            m_pipeline_layout = pipeline_layout;
        }

        auto config = PreparedPipelineConfiguration(m_configuration.pipeline, m_pipeline_layout);

        vk::GraphicsPipelineCreateInfo graphics_pipeline_info = {
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

        {
            auto [result, graphics_pipeline] = m_device.createGraphicsPipeline(nullptr, graphics_pipeline_info);
            if (result != vk::Result::eSuccess)
                throw VulkanException((uint32_t)result, "Failed to create graphics pipeline");
            m_logger->info("Created graphics pipeline");

            m_pipeline = graphics_pipeline;
        }
    }

    void RenderManager::create_framebuffers()
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
                .width           = m_configuration.swapchain.extent.width,
                .height          = m_configuration.swapchain.extent.height,
                .layers          = m_configuration.swapchain.image_layers,
            };

            auto [result, framebuffer] = m_device.createFramebuffer(framebuffer_create_info);
            if (result != vk::Result::eSuccess)
                throw VulkanException((uint32_t)result, "Failed to create framebuffer");

            m_framebuffers[i] = framebuffer;
        }

        m_logger->info("Created framebuffer");
    }

    void RenderManager::create_command_pool()
    {
        vk::CommandPoolCreateInfo create_info = {
            .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = m_device_manager->m_graphics_queue.index,
        };

        auto [result, command_pool] = m_device.createCommandPool(create_info);
        if (result != vk::Result::eSuccess)
            throw VulkanException((uint32_t)result, "Failed to create command pool");
        m_logger->info("Created command pool");

        m_command_pool = command_pool;
    }

    void RenderManager::allocate_command_buffers()
    {
        vk::CommandBufferAllocateInfo allocate_info = {
            .commandPool        = m_command_pool,
            .level              = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = IN_FLIGHT,
        };

        auto [result, command_buffers] = m_device.allocateCommandBuffers(allocate_info);
        if (result != vk::Result::eSuccess)
            throw VulkanException((uint32_t)result, "Failed to allocate command buffers");
        m_logger->info("Allocated {} command buffers", command_buffers.size());

        m_command_buffers = move(command_buffers);
    }

    void RenderManager::create_sync_primitives()
    {
        m_sync.init(m_device);
    }

    void RenderManager::record_command_buffer(vk::CommandBuffer buffer, uint32_t image_index)
    {
        vk::CommandBufferBeginInfo buffer_begin = {
            .flags            = {},
            .pInheritanceInfo = nullptr,
        };

        auto result = buffer.begin(buffer_begin);
        if (result != vk::Result::eSuccess)
            throw VulkanException((uint32_t)result, "Failed to begin recording command buffer");

        vk::ClearValue clear_value = vk::ClearValue {vk::ClearColorValue {array {0.0f, 0.0f, 0.0f, 1.0f}}};

        vk::RenderPassBeginInfo render_pass_begin = {
            .renderPass      = m_render_pass,
            .framebuffer     = m_framebuffers[image_index],
            .renderArea      = {.offset = {0, 0}, .extent = m_configuration.swapchain.extent},
            .clearValueCount = 1,
            .pClearValues    = &clear_value,
        };

        vk::Viewport viewport = {
            .x        = 0.0,
            .y        = 0.0,
            .width    = (float)m_configuration.swapchain.extent.width,
            .height   = (float)m_configuration.swapchain.extent.height,
            .minDepth = 0.0,
            .maxDepth = 1.0,
        };

        vk::Rect2D scissor = {
            .offset = {0, 0},
            .extent = m_configuration.swapchain.extent,
        };

        buffer.beginRenderPass(render_pass_begin, vk::SubpassContents::eInline);
        buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
        buffer.setViewport(0, viewport);
        buffer.setScissor(0, scissor);
        buffer.draw(3, 1, 0, 0);
        buffer.endRenderPass();
        result = buffer.end();

        if (result != vk::Result::eSuccess)
            throw VulkanException((uint32_t)result, "Failed to record command buffer");
    }

    void GpuSync::init(vk::Device device)
    {
        vk::SemaphoreCreateInfo sem = {};
        vk::FenceCreateInfo     fen = {
                .flags = vk::FenceCreateFlagBits::eSignaled,
        };

        {
            auto [result, semaphore] = device.createSemaphore(sem);
            if (result != vk::Result::eSuccess)
                throw VulkanException((uint32_t)result, "Failed to create semaphore");

            image_available = semaphore;
        }
        {
            auto [result, semaphore] = device.createSemaphore(sem);
            if (result != vk::Result::eSuccess)
                throw VulkanException((uint32_t)result, "Failed to create semaphore");

            render_finished = semaphore;
        }
        {
            auto [result, fence] = device.createFence(fen);
            if (result != vk::Result::eSuccess)
                throw VulkanException((uint32_t)result, "Failed to create fence");

            in_flight = fence;
        }
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
} // namespace engine