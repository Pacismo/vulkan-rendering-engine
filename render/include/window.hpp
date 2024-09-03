#pragma once
#include "version.hpp"

#include <GLFW/glfw3.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <string_view>

namespace engine
{
    class Window
    {
        using UniqueRenderManager   = std::unique_ptr<class RenderManager>;

      public:
        /// Create a new window, creating new instance and device configurations.
        Window(std::string_view title, int32_t width, int32_t height, std::string_view application_name = "app_runtime",
               Version application_version = Version {.major = 0, .minor = 1, .patch = 0, .variant = 0});
        /// Create a new window, deriving the instance and device configurations from an existing window.
        Window(std::string_view title, int32_t width, int32_t height, const Window &other);

        void show();
        void hide();

        void set_title(std::string_view new_title);

        virtual void process() = 0;

        void run();

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
        UniqueRenderManager             m_render_manager   = {};
    };
} // namespace engine