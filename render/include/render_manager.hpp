#pragma once
#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>
#include <memory>
#include <spdlog/spdlog.h>

namespace engine
{
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

        RenderManager(SharedDeviceManager device_manager, GLFWwindow *window);
        ~RenderManager();

        static Unique new_unique(SharedDeviceManager device_manager, GLFWwindow *window);

        // No copying
        RenderManager(const RenderManager &)            = delete;
        RenderManager &operator=(const RenderManager &) = delete;

        RenderManager(RenderManager &&) noexcept;
        RenderManager &operator=(RenderManager &&) noexcept;

      private:
        std::shared_ptr<spdlog::logger> m_logger           = {};
        SharedInstanceManager           m_instance_manager = {};
        SharedDeviceManager             m_device_manager   = {};
        vk::SurfaceKHR                  m_surface          = {};
    };
} // namespace engine