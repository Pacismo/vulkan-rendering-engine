#pragma once
#include "backend/image_allocation.hpp"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

namespace engine
{
    struct SwapchainSupportDetails
    {
        vk::SurfaceCapabilitiesKHR        capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR>   modes;

        /// Queries support details
        static SwapchainSupportDetails query(vk::PhysicalDevice device, vk::SurfaceKHR surface);
        /// Ensures that there is at least one surface format and one surface present mode
        static bool                    supported(vk::PhysicalDevice device, vk::SurfaceKHR surface);
    };

    struct SwapchainConfiguration
    {
        vk::Format         format       = {};
        vk::ColorSpaceKHR  color_space  = {};
        vk::PresentModeKHR present_mode = {};
        vk::Extent2D       extent       = {};
        uint32_t           image_count  = {};
        uint32_t           image_layers = {};
    };

    struct Image
    {
        vk::Image     handle = nullptr;
        vk::ImageView view   = nullptr;

        inline operator vk::Image() { return handle; }
        inline operator vk::ImageView() { return view; }
    };

    struct Framebuffer
    {
        Image           color  = {};
        ImageAllocation depth  = {};
        vk::Framebuffer handle = nullptr;

        inline operator vk::Framebuffer() { return handle; }
    };

    class SwapchainManager final
    {
        using SharedDeviceManager = std::shared_ptr<class RenderDeviceManager>;

      public:
        SwapchainManager();
        SwapchainManager(SharedDeviceManager device_manager, vk::SurfaceKHR surface, SwapchainConfiguration config);
        ~SwapchainManager();

        /// Returns `true` if the swapchain is valid
        bool recreate_swapchain(SwapchainConfiguration config);

        void init(SharedDeviceManager device_manager, vk::SurfaceKHR surface, SwapchainConfiguration config);
        void destroy();

        operator bool() const noexcept;
        operator vk::SwapchainKHR() const noexcept;

        Framebuffer &operator[](size_t i);

      private:
        void create_swapchain();
        void get_swapchain_images();
        void destroy_swapchain();

        SharedDeviceManager m_device_manager = nullptr;
        vk::Device          m_device         = nullptr;
        vk::SurfaceKHR      m_surface        = nullptr;

      public:
        vk::RenderPass           render_pass   = nullptr;
        vk::SwapchainKHR         swapchain     = nullptr;
        std::vector<Framebuffer> images        = {};
        SwapchainConfiguration   configuration = {};
        vk::Format               depth_format  = {};
    };
} // namespace engine