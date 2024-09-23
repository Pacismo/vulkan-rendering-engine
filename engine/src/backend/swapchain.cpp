#include "backend/swapchain.hpp"
#include "backend/device_manager.hpp"
#include "exceptions.hpp"
#include <array>

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
    }

    void SwapchainManager::init(SharedDeviceManager device_manager, vk::SurfaceKHR surface,
                                SwapchainConfiguration config)
    {
        m_device_manager = device_manager;
        m_device         = device_manager->device;
        m_surface        = surface;
        configuration    = config;

        create_swapchain();

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

        render_pass = m_device.createRenderPass(render_pass_create_info);

        get_swapchain_images();
    }

    void SwapchainManager::destroy()
    {
        destroy_swapchain();

        if (render_pass)
            m_device.destroyRenderPass(render_pass);

        m_device.destroySwapchainKHR(swapchain);

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

        swapchain = m_device.createSwapchainKHR(swapchain_create_info);
    }

    void SwapchainManager::get_swapchain_images()
    {
        auto image_handles = m_device.getSwapchainImagesKHR(swapchain);
        images.resize(image_handles.size());

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

            images[i].handle = image_handles[i];
            images[i].view   = m_device.createImageView(create_info);

            std::array<vk::ImageView, 1> attachments = {
                images[i].view,
            };

            vk::FramebufferCreateInfo framebuffer_create_info = {
                .renderPass      = render_pass,
                .attachmentCount = attachments.size(),
                .pAttachments    = attachments.data(),
                .width           = configuration.extent.width,
                .height          = configuration.extent.height,
                .layers          = configuration.image_layers,
            };

            images[i].framebuffer = m_device.createFramebuffer(framebuffer_create_info);
        }
    }

    void SwapchainManager::destroy_swapchain()
    {
        for (auto &image : images) {
            m_device.destroyFramebuffer(image.framebuffer);
            m_device.destroyImageView(image.view);
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

    Image &SwapchainManager::operator[](size_t i)
    {
        return images[i];
    }
} // namespace engine