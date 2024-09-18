#pragma once
#include "version.hpp"

#include <GLFW/glfw3.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <string_view>

#ifdef VULKAN_HPP
#    include "backend/vulkan_backend.hpp"
#endif

namespace engine
{
    class Window
    {
        friend class VulkanBackend;
        using SharedRenderManager = std::shared_ptr<class VulkanBackend>;

      public:
        /// Create a new window, creating new instance and device configurations.
        Window(std::string_view title, int32_t width, int32_t height, std::string_view application_name = "app_runtime",
               Version application_version = Version {.major = 0, .minor = 1, .patch = 0, .variant = 0});
        /// Create a new window, deriving the instance and device configurations from an existing window.
        Window(std::string_view title, int32_t width, int32_t height, const Window &other);

        void show();
        void hide();

        void set_title(std::string_view new_title);

        SharedRenderManager get_render_backend();

        virtual void handle_draw(struct DrawingContext &context) = 0;

        virtual void process(double delta);
        virtual void physics_process(double delta);

        void run(double pproc_freq = 20.0);

        virtual ~Window();

        // No copying
        Window(const Window &)            = delete;
        Window &operator=(const Window &) = delete;

        Window(Window &&) noexcept;
        Window &operator=(Window &&) noexcept;

      protected:
        GLFWwindow *mp_window = {};

      private:
        std::shared_ptr<spdlog::logger> m_logger;
        SharedRenderManager             m_render_manager = {};
    };
} // namespace engine