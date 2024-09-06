#pragma once
#include <vulkan/vulkan.hpp>

#include "version.hpp"
#include <GLFW/glfw3.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <string_view>

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

    struct RenderConfiguration
    {
        SwapchainConfiguration swapchain = {};
        PipelineConfiguration  pipeline  = {};
    };

    struct GpuSync
    {
        vk::Semaphore image_available = {};
        vk::Semaphore render_finished = {};
        vk::Fence     in_flight       = {};

        void init(vk::Device device);
        void destroy(vk::Device device);
    };

    /// Manages the data pertaining to a rendering pipeline.
    ///
    /// Must be owned by the window using it.
    class RenderManager final
    {
        friend class Window;

        using SharedInstanceManager = std::shared_ptr<class VulkanInstanceManager>;
        using SharedDeviceManager   = std::shared_ptr<class RenderDeviceManager>;

      public:
        using Unique = std::unique_ptr<RenderManager>;

        RenderManager(std::string_view application_name, Version application_version, GLFWwindow *window);
        RenderManager(const RenderManager &other, GLFWwindow *window);
        ~RenderManager();

        /// Recreate the pipeline
        void recreate_pipeline();

        /// Make a new unique pointer to a RenderManager
        static Unique new_unique(std::string_view application_name, Version application_version, GLFWwindow *window);

        /// Clone the instance/device managers to create a new RenderManager
        ///
        /// The device from `other` must be compatible with the surface created with the window
        static Unique new_unique(const Unique &other, GLFWwindow *window);

        /// Render a frame
        void render_frame();

        /// Wait until the device is idle
        void wait_idle();

        // No copying
        RenderManager(const RenderManager &)            = delete;
        RenderManager &operator=(const RenderManager &) = delete;

        RenderManager(RenderManager &&) noexcept;
        RenderManager &operator=(RenderManager &&) noexcept;

      private:
        std::shared_ptr<spdlog::logger> m_logger           = {};
        SharedInstanceManager           m_instance_manager = {};
        SharedDeviceManager             m_device_manager   = {};

        static constexpr uint32_t      IN_FLIGHT         = 1;
        GLFWwindow                    *m_window          = {};
        vk::Device                     m_device          = {};
        vk::Queue                      m_graphics_queue  = {};
        vk::Queue                      m_present_queue   = {};
        vk::SurfaceKHR                 m_surface         = {};
        vk::SwapchainKHR               m_swapchain       = {};
        std::vector<vk::Image>         m_images          = {};
        std::vector<vk::ImageView>     m_image_views     = {};
        vk::RenderPass                 m_render_pass     = {};
        vk::PipelineLayout             m_pipeline_layout = {};
        vk::Pipeline                   m_pipeline        = {};
        std::vector<vk::Framebuffer>   m_framebuffers    = {};
        vk::CommandPool                m_command_pool    = {};
        std::vector<vk::CommandBuffer> m_command_buffers = {};
        GpuSync                        m_sync            = {};

        RenderConfiguration m_configuration = {};

        // Pipeline initialization functions

        /// Initialize the configuration of the pipeline
        void initialize_pipeline_configuration();

        /// Get the images owned by the swapchain
        void get_images();

        /// Create the render pass
        void create_render_pass();
        /// Create a new swapchain
        void create_swapchain();
        /// Create the pipeline
        void create_pipeline();
        /// Create a rendering pipeline
        void create_render_pipeline();
        /// Create the framebuffers
        void create_framebuffers();
        /// Create the command pool
        void create_command_pool();
        /// Allocates the command buffers
        void allocate_command_buffers();
        /// Create the synchronization primitives
        void create_sync_primitives();

        void record_command_buffer(vk::CommandBuffer buffer, uint32_t image_index);

        // Pipeline destruction function

        /// Cleans the pipeline while keeping the swapchain and surface untouched
        void clean_pipeline();
    };
} // namespace engine