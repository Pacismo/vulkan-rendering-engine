#pragma once
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "render_backend.hpp"
#include "version.hpp"
#include <GLFW/glfw3.h>
#include <forward_list>
#include <memory>
#include <optional>
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

    /// Manages the data pertaining to a rendering pipeline.
    ///
    /// Must be owned by the window using it.
    class VulkanBackend final : public RenderBackend
    {
        friend class Window;
        friend class GpuBufferWriter;

        using SharedInstanceManager = std::shared_ptr<class VulkanInstanceManager>;
        using SharedDeviceManager   = std::shared_ptr<class RenderDeviceManager>;
        using Shared                = std::shared_ptr<VulkanBackend>;

        struct VertexBufferAllocation final : public MeshHandle
        {
            // Does not own data
            VulkanBackend *backend;
            VmaAllocation  alloc;
            vk::Buffer     buffer;
            vk::DeviceSize offset;
            vk::DeviceSize size;
            uint32_t       vertex_count;

            bool is_visible;

            VertexBufferAllocation(VulkanBackend *backend, VmaAllocation alloc, vk::Buffer buffer,
                                   vk::DeviceSize offset, vk::DeviceSize size, uint32_t vertices);
            void destroy(bool destructor = false);
            ~VertexBufferAllocation() override;

            void visible(bool value) override;
            bool visible() const override;
        };

        struct StagingBuffer
        {
            static constexpr uint32_t SIZE = 8192;

            VmaAllocation     alloc;
            vk::Buffer        buffer;
            vk::CommandBuffer cmd;
            vk::Fence         transfer_fence;
            bool              is_coherent;
            void             *p_mapping;

            operator void *();

            void init(VmaAllocator allocator, vk::CommandBuffer cmd, vk::Fence fence);
            void flush(VmaAllocator allocator, vk::DeviceSize offset = 0, vk::DeviceSize length = SIZE);
            void transfer(vk::Buffer dst, vk::Queue queue, uint32_t src_offset, uint32_t dst_offset, uint32_t size);
            void reset(vk::Device device);
            void wait(vk::Device device);
            void deinit(vk::Device device, vk::CommandPool cmd_pool, VmaAllocator allocator);
        };

      public:
        MeshHandlePtr load(std::span<primitives::Vertex> vertices) override;

        VulkanBackend(std::string_view application_name, Version application_version, GLFWwindow *window);
        VulkanBackend(const VulkanBackend &other, GLFWwindow *window);
        ~VulkanBackend() override;

        /// Make a new shared pointer to a RenderManager
        static Shared new_shared(std::string_view application_name, Version application_version, GLFWwindow *window);
        /// Clone the instance/device managers to create a new RenderManager
        ///
        /// The device from `other` must be compatible with the surface created with the window
        static Shared new_shared(const Shared &other, GLFWwindow *window);

        /// Render a frame
        void render_frame();

        /// Wait until the device is idle
        void wait_idle();

        /// Recreate the swapchain
        bool recreate_swapchain();

        // No copying
        VulkanBackend(const VulkanBackend &)            = delete;
        VulkanBackend &operator=(const VulkanBackend &) = delete;

        VulkanBackend(VulkanBackend &&) noexcept;
        VulkanBackend &operator=(VulkanBackend &&) noexcept;

      private:
        std::shared_ptr<spdlog::logger> m_logger           = {};
        SharedInstanceManager           m_instance_manager = {};
        SharedDeviceManager             m_device_manager   = {};

        static constexpr uint32_t                   IN_FLIGHT         = 2;
        uint32_t                                    m_frame_index     = 0;
        GLFWwindow                                 *m_window          = {};
        vk::Device                                  m_device          = {};
        vk::Queue                                   m_graphics_queue  = {};
        vk::Queue                                   m_present_queue   = {};
        vk::SurfaceKHR                              m_surface         = {};
        vk::SwapchainKHR                            m_swapchain       = {};
        std::vector<vk::Image>                      m_images          = {};
        std::vector<vk::ImageView>                  m_image_views     = {};
        vk::RenderPass                              m_render_pass     = {};
        vk::PipelineLayout                          m_pipeline_layout = {};
        vk::Pipeline                                m_pipeline        = {};
        std::vector<vk::Framebuffer>                m_framebuffers    = {};
        vk::CommandPool                             m_command_pool    = {};
        std::vector<vk::CommandBuffer>              m_command_buffers = {};
        std::vector<GpuSync>                        m_sync            = {};
        vk::ShaderModule                            m_vertex_shader   = {};
        vk::ShaderModule                            m_fragment_shader = {};
        VmaAllocator                                m_allocator       = {};
        std::forward_list<VertexBufferAllocation *> m_loaded_meshes   = {};
        StagingBuffer                               m_staging_buffer  = {};

        SwapchainConfiguration m_swapchain_config    = {};
        bool                   m_framebuffer_resized = false;
        bool                   m_valid_framebuffer   = {};

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
        /// Initialize the allocator
        void initialize_device_memory_allocator();

        void record_command_buffer(vk::CommandBuffer buffer, uint32_t image_index);

        /// Destroy the swapchain, its images, and the framebuffers
        void destroy_swapchain();
    };
} // namespace engine