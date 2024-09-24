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
#include "swapchain.hpp"
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
        using Unique                = std::unique_ptr<VulkanBackend>;

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

        ~VulkanBackend();

        /// Make a new shared pointer to a RenderManager
        static Unique new_unique(std::string_view application_name, Version application_version, GLFWwindow *window);
        /// Clone the instance/device managers to create a new RenderManager
        ///
        /// The device from `other` must be compatible with the surface created with the window
        static Unique new_from(const VulkanBackend &other, GLFWwindow *window);

        /// Wait until the device is idle
        void wait_idle();

        /// Recreate the swapchain
        bool recreate_swapchain();

        // No copying
        VulkanBackend(const VulkanBackend &)            = delete;
        VulkanBackend &operator=(const VulkanBackend &) = delete;

        // No moving
        VulkanBackend(VulkanBackend &&) noexcept            = delete;
        VulkanBackend &operator=(VulkanBackend &&) noexcept = delete;

        std::shared_ptr<spdlog::logger> m_logger           = {};
        SharedInstanceManager           m_instance_manager = {};
        SharedDeviceManager             m_device_manager   = {};

        uint32_t                         m_frame_index               = 0;
        GLFWwindow                      *m_window                    = {};
        vk::Device                       m_device                    = {};
        std::shared_ptr<VulkanAllocator> m_allocator                 = {};
        vk::Queue                        m_graphics_queue            = {};
        vk::Queue                        m_present_queue             = {};
        vk::SurfaceKHR                   m_surface                   = {};
        SwapchainManager                 m_swapchain                 = {};
        vk::PipelineLayout               m_pipeline_layout           = {};
        vk::Pipeline                     m_gouraud_pipeline          = {};
        CommandPoolManager               m_command_pool              = {};
        DescriptorPoolManager            m_descriptor_pool           = {};
        std::vector<FrameSet>            m_frame_sets                = {};
        vk::ShaderModule                 m_vertex_shader             = {};
        vk::ShaderModule                 m_fragment_shader           = {};
        vk::DescriptorSetLayout          m_uniform_descriptor_layout = {};
        StagingBuffer                    m_staging_buffer            = {};

        float                                                            m_fov        = DEFAULT_FOV;
        glm::mat4                                                        m_camera     = {1.0};
        TypedHostVisibleBufferAllocation<ViewProjectionUniform[MAX_IN_FLIGHT]> m_vp_uniform = {};

        bool m_framebuffer_resized = false;

      private:
        VulkanBackend(std::string_view application_name, Version application_version, GLFWwindow *window);
        VulkanBackend(const VulkanBackend &other, GLFWwindow *window);

        // Pipeline initialization functions

        /// Create the pipeline
        void create_pipeline();
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
        /// Create the command pool
        void create_command_pool();
        /// Initialize the frame sets
        void initialize_frame_sets();
        /// Initialize the allocator
        void initialize_device_memory_allocator();
        /// Initialize other data
        void finalize_init();

        void initialize_command_buffer(vk::CommandBuffer buffer, uint32_t image_index);
    };
} // namespace engine
