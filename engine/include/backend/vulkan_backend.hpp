#pragma once
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "allocation.hpp"
#include "allocator.hpp"
#include "command_pool.hpp"
#include "constants.hpp"
#include "descriptor_pool.hpp"
#include "drawables/GouraudMesh.hpp"
#include "drawables/drawing_context.hpp"
#include "version.hpp"
#include "vertex.hpp"
#include <GLFW/glfw3.h>
#include <memory>
#include <optional>
#include <span>
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

    struct GpuSync
    {
        vk::Semaphore image_available = {};
        vk::Semaphore render_finished = {};
        vk::Fence     in_flight       = {};

        void init(vk::Device device);
        void destroy(vk::Device device);
    };

    struct ViewProjectionUniform
    {
        glm::mat4 view       = {};
        glm::mat4 projection = {};
    };

    /// Manages the data pertaining to a rendering pipeline.
    ///
    /// Must be owned by the window using it.
    class VulkanBackend final
    {
        friend class Window;
        friend class GpuBufferWriter;

        using SharedInstanceManager = std::shared_ptr<class VulkanInstanceManager>;
        using SharedDeviceManager   = std::shared_ptr<class RenderDeviceManager>;
        using Shared                = std::shared_ptr<VulkanBackend>;

        struct StagingBuffer
        {
            static constexpr uint32_t SIZE = 8192;

            VmaAllocation     alloc;
            vk::Buffer        buffer;
            vk::CommandBuffer cmd;
            vk::Fence         transfer_fence;
            bool              is_coherent;
            void             *p_mapping;

            operator uint8_t *();

            void init(VmaAllocator allocator, vk::CommandBuffer cmd, vk::Fence fence);
            void flush(VmaAllocator allocator, vk::DeviceSize offset = 0, vk::DeviceSize length = SIZE);
            void transfer(vk::Buffer dst, vk::Queue queue, uint32_t src_offset, uint32_t dst_offset, uint32_t size);
            void reset(vk::Device device);
            void wait(vk::Device device);
            void deinit(vk::Device device, vk::CommandPool cmd_pool, VmaAllocator allocator);
        };

        struct FrameSet
        {
            vk::CommandBuffer                              command_buffer;
            GpuSync                                        sync;
            std::array<vk::DescriptorSet, MAX_DESCRIPTORS> descriptors;
        };

      public:
        void update_fov(float fov);
        void update_view(const glm::mat4 &transformation);

        std::shared_ptr<class GouraudMesh> load(std::span<primitives::GouraudVertex> vertices,
                                                std::span<uint32_t>                  indices);

        std::optional<DrawingContext> begin_draw();
        void                          end_draw(DrawingContext &context);

        VulkanBackend(std::string_view application_name, Version application_version, GLFWwindow *window);
        VulkanBackend(const VulkanBackend &other, GLFWwindow *window);
        ~VulkanBackend();

        /// Make a new shared pointer to a RenderManager
        static Shared new_shared(std::string_view application_name, Version application_version, GLFWwindow *window);
        /// Clone the instance/device managers to create a new RenderManager
        ///
        /// The device from `other` must be compatible with the surface created with the window
        static Shared new_shared(const Shared &other, GLFWwindow *window);

        /// Wait until the device is idle
        void wait_idle();

        /// Recreate the swapchain
        bool recreate_swapchain();

        // No copying
        VulkanBackend(const VulkanBackend &)            = delete;
        VulkanBackend &operator=(const VulkanBackend &) = delete;

        VulkanBackend(VulkanBackend &&) noexcept;
        VulkanBackend &operator=(VulkanBackend &&) noexcept;

        std::shared_ptr<spdlog::logger> m_logger           = {};
        SharedInstanceManager           m_instance_manager = {};
        SharedDeviceManager             m_device_manager   = {};

        uint32_t                         m_frame_index               = 0;
        GLFWwindow                      *m_window                    = {};
        vk::Device                       m_device                    = {};
        vk::Queue                        m_graphics_queue            = {};
        vk::Queue                        m_present_queue             = {};
        vk::SurfaceKHR                   m_surface                   = {};
        vk::SwapchainKHR                 m_swapchain                 = {};
        std::vector<vk::Image>           m_images                    = {};
        std::vector<vk::ImageView>       m_image_views               = {};
        vk::RenderPass                   m_render_pass               = {};
        vk::PipelineLayout               m_pipeline_layout           = {};
        vk::Pipeline                     m_gouraud_pipeline          = {};
        vk::Pipeline                     m_textured_pipeline         = {};
        std::vector<vk::Framebuffer>     m_framebuffers              = {};
        CommandPoolManager               m_command_pool              = {};
        DescriptorPoolManager            m_descriptor_pool           = {};
        std::vector<FrameSet>            m_frame_sets                = {};
        vk::ShaderModule                 m_vertex_shader             = {};
        vk::ShaderModule                 m_fragment_shader           = {};
        vk::DescriptorSetLayout          m_uniform_descriptor_layout = {};
        std::shared_ptr<VulkanAllocator> m_allocator                 = {};
        StagingBuffer                    m_staging_buffer            = {};

        float                                                            m_fov        = DEFAULT_FOV;
        glm::mat4                                                        m_camera     = {1.0};
        TypedHostVisibleAllocation<ViewProjectionUniform[MAX_IN_FLIGHT]> m_vp_uniform = {};

        SwapchainConfiguration m_swapchain_config    = {};
        bool                   m_framebuffer_resized = false;
        bool                   m_valid_framebuffer   = {};

      private:
        static void handle_framebuffer_resize(GLFWwindow *window, int width, int height);

        // Pipeline initialization functions

        /// Get the images owned by the swapchain
        void get_images();

        /// Create the pipeline
        void create_pipeline();
        /// Create the render pass
        void create_render_pass();
        /// Create a new swapchain
        void create_swapchain();
        /// Load the shaders
        void load_shaders();
        /// Create the descriptor sets
        void create_descriptor_set_layout();
        /// Create the descriptor pool
        void create_descriptor_pools();
        /// Create a rendering pipeline
        void create_render_pipeline();
        /// Create the framebuffers
        void create_framebuffers();
        /// Create the command pool
        void create_command_pool();
        /// Initialize the frame sets
        void initialize_frame_sets();
        /// Initialize the allocator
        void initialize_device_memory_allocator();
        /// Initialize other data
        void finalize_init();

        void initialize_command_buffer(vk::CommandBuffer buffer, uint32_t image_index);

        /// Destroy the swapchain, its images, and the framebuffers
        void destroy_swapchain();
    };
} // namespace engine
