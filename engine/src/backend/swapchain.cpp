#include "backend/swapchain.hpp"
#include "backend/device_manager.hpp"
#include "exceptions.hpp"
#include <array>

static const std::array<vk::Format, 3> SUPPORTED_DEPTH_FORMATS = {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint,
                                                                  vk::Format::eD24UnormS8Uint};

static bool has_stencil_component(vk::Format format)
{
    return (format == vk::Format::eD32SfloatS8Uint) || (format == vk::Format::eD24UnormS8Uint);
}

namespace engine
{
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

    SwapchainManager::SwapchainManager() { }

    SwapchainManager::SwapchainManager(SharedDeviceManager device_manager, vk::SurfaceKHR surface,
                                       SwapchainConfiguration config)
    {
        init(device_manager, surface, configuration);
    }

    SwapchainManager::~SwapchainManager()
    {
        destroy();
    }

    bool SwapchainManager::recreate_swapchain(SwapchainConfiguration config)
    {
        m_device.waitIdle();
        if (swapchain)
            destroy_swapchain();

        if (config.extent.width == 0 || config.extent.height == 0)
            return false;

        configuration = config;
        create_swapchain();
        get_swapchain_images();
        return true;
    }

    void SwapchainManager::init(SharedDeviceManager device_manager, vk::SurfaceKHR surface,
                                SwapchainConfiguration config)
    {
        m_device_manager = device_manager;
        m_device         = device_manager->device;
        m_surface        = surface;
        configuration    = config;

        create_swapchain();

        depth_format = m_device_manager->find_supported_format(SUPPORTED_DEPTH_FORMATS, vk::ImageTiling::eOptimal,
                                                               vk::FormatFeatureFlagBits::eDepthStencilAttachment);

        vk::AttachmentDescription color_attachment = {
            .format         = configuration.format,
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

        vk::AttachmentDescription depth_attachment = {
            .format         = depth_format,
            .samples        = vk::SampleCountFlagBits::e1,
            .loadOp         = vk::AttachmentLoadOp::eClear,
            .storeOp        = vk::AttachmentStoreOp::eDontCare,
            .stencilLoadOp  = vk::AttachmentLoadOp::eDontCare,
            .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
            .initialLayout  = vk::ImageLayout::eUndefined,
            .finalLayout    = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        };

        vk::AttachmentReference depth_attachment_reference = {
            .attachment = 1,
            .layout     = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        };

        std::array<vk::AttachmentDescription, 2> attachment_descriptions = {
            color_attachment,
            depth_attachment,
        };

        vk::SubpassDescription subpass_description = {
            .pipelineBindPoint       = vk::PipelineBindPoint::eGraphics,
            .colorAttachmentCount    = 1,
            .pColorAttachments       = &color_attachment_reference,
            .pDepthStencilAttachment = &depth_attachment_reference,
        };

        using vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        using vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eEarlyFragmentTests;

        vk::SubpassDependency subpass = {
            .srcSubpass    = vk::SubpassExternal,
            .dstSubpass    = 0,
            .srcStageMask  = eColorAttachmentOutput | eEarlyFragmentTests,
            .dstStageMask  = eColorAttachmentOutput | eEarlyFragmentTests,
            .srcAccessMask = vk::AccessFlagBits::eNone,
            .dstAccessMask = eColorAttachmentWrite | eDepthStencilAttachmentWrite,
        };

        vk::RenderPassCreateInfo render_pass_create_info = {
            .attachmentCount = attachment_descriptions.size(),
            .pAttachments    = attachment_descriptions.data(),
            .subpassCount    = 1,
            .pSubpasses      = &subpass_description,
            .dependencyCount = 1,
            .pDependencies   = &subpass,
        };

        render_pass = m_device.createRenderPass(render_pass_create_info);

        get_swapchain_images();
    }

    void SwapchainManager::destroy()
    {
        destroy_swapchain();

        if (render_pass)
            m_device.destroyRenderPass(render_pass);

        if (swapchain)
            m_device.destroySwapchainKHR(swapchain);

        images.clear();

        swapchain        = nullptr;
        render_pass      = nullptr;
        m_device         = nullptr;
        m_surface        = nullptr;
        m_device_manager = nullptr;
    }

    void SwapchainManager::create_swapchain()
    {
        auto swapchain_support_details = SwapchainSupportDetails::query(m_device_manager->physical_device, m_surface);

        uint32_t queue_families[2] = {m_device_manager->graphics_queue.index, m_device_manager->present_queue.index};

        bool same_queues = queue_families[0] == queue_families[1];

        vk::SwapchainCreateInfoKHR swapchain_create_info = {
            .surface               = m_surface,
            .minImageCount         = configuration.image_count,
            .imageFormat           = configuration.format,
            .imageColorSpace       = configuration.color_space,
            .imageExtent           = configuration.extent,
            .imageArrayLayers      = configuration.image_layers,
            .imageUsage            = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode      = same_queues ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
            .queueFamilyIndexCount = same_queues ? 0u : 2u,
            .pQueueFamilyIndices   = same_queues ? nullptr : queue_families,
            .preTransform          = swapchain_support_details.capabilities.currentTransform,
            .compositeAlpha        = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode           = configuration.present_mode,
            .clipped               = true,
            .oldSwapchain          = swapchain, // Should be NULL for the first call
        };
        vk::SwapchainKHR old_swapchain = swapchain;

        swapchain = m_device.createSwapchainKHR(swapchain_create_info);

        if (old_swapchain)
            m_device.destroySwapchainKHR(old_swapchain);
    }

    void SwapchainManager::get_swapchain_images()
    {
        auto image_handles = m_device.getSwapchainImagesKHR(swapchain);
        images.resize(image_handles.size());

        // TODO: create allocator and create depth attachments
        auto allocator = VulkanAllocator::new_shared(m_device_manager);

        for (size_t i = 0; i < image_handles.size(); ++i) {
            vk::ImageViewCreateInfo create_info = {
                .image            = image_handles[i],
                .viewType         = vk::ImageViewType::e2D,
                .format           = configuration.format,
                .components       = {.r = vk::ComponentSwizzle::eIdentity,
                                     .g = vk::ComponentSwizzle::eIdentity,
                                     .b = vk::ComponentSwizzle::eIdentity,
                                     .a = vk::ComponentSwizzle::eIdentity},
                .subresourceRange = {.aspectMask     = vk::ImageAspectFlagBits::eColor,
                                     .baseMipLevel   = 0,
                                     .levelCount     = 1,
                                     .baseArrayLayer = 0,
                                     .layerCount     = configuration.image_layers},
            };

            images[i].color.handle = image_handles[i];
            images[i].color.view   = m_device.createImageView(create_info);

            ImageAllocationInfo iainfo = {
                .usage  = vk::ImageUsageFlagBits::eDepthStencilAttachment,
                .width  = configuration.extent.width,
                .height = configuration.extent.height,
                .format = depth_format,
            };
            iainfo.view_subresource_range.aspectMask = vk::ImageAspectFlagBits::eDepth;

            images[i].depth = ImageAllocation(allocator, iainfo);

            std::array<vk::ImageView, 2> attachments = {
                images[i].color,
                images[i].depth,
            };

            vk::FramebufferCreateInfo framebuffer_create_info = {
                .renderPass      = render_pass,
                .attachmentCount = attachments.size(),
                .pAttachments    = attachments.data(),
                .width           = configuration.extent.width,
                .height          = configuration.extent.height,
                .layers          = configuration.image_layers,
            };

            images[i].handle = m_device.createFramebuffer(framebuffer_create_info);
        }
    }

    void SwapchainManager::destroy_swapchain()
    {
        for (auto &image : images) {
            m_device.destroyFramebuffer(image.handle);
            m_device.destroyImageView(image.color);
        }

        images.clear();
    }

    SwapchainManager::operator bool() const noexcept
    {
        return images.size() > 0;
    }

    SwapchainManager::operator vk::SwapchainKHR() const noexcept
    {
        return swapchain;
    }

    Framebuffer &SwapchainManager::operator[](size_t i)
    {
        return images[i];
    }
} // namespace engine
