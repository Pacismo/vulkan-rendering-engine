#pragma once
#include "version.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>
#include <spdlog/spdlog.h>
#include <string_view>

#include "input/controller.hpp"
#include "input/keyboard.hpp"
#include "input/mouse.hpp"

#include "gui/imgui_manager.hpp"

#ifdef VULKAN_HPP
#    include "backend/vulkan_backend.hpp"
#endif

namespace engine
{
    /**
     * @brief Base class for all windows.
     *
     * Fully constructs the necessary components to render objects to the screen.
     */
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

        glm::vec2 get_axis(KeyboardKey px, KeyboardKey nx, KeyboardKey py, KeyboardKey ny);
        float     get_magnitude(KeyboardKey p, KeyboardKey n);

        void close(bool should_close = true);

        void set_title(std::string_view new_title);

        SharedRenderManager get_render_backend();

        virtual void handle_draw(struct DrawingContext &context) = 0;

        virtual void on_key_action(KeyboardKey key, ModifierKey modifiers, KeyAction action, int scancode);
        virtual void on_mouse_button_action(MouseButton button, ModifierKey modifiers, KeyAction action);
        virtual void on_cursor_motion(double x, double y, double dx, double dy);

        virtual void process(double delta);
        virtual void physics_process(double delta);

        void run(double pproc_freq = 20.0);

        virtual ~Window();

        // No copying
        Window(const Window &)            = delete;
        Window &operator=(const Window &) = delete;

        Window(Window &&) noexcept            = delete;
        Window &operator=(Window &&) noexcept = delete;

      protected:
        GLFWwindow  *m_window        = {};
        ImGuiManager m_imgui_manager = {};

        double last_mouse_x = 0.0;
        double last_mouse_y = 0.0;

      private:
        std::shared_ptr<spdlog::logger> m_logger;
        SharedRenderManager             m_render_manager = {};

        void set_glfw_callbacks();

        static void key_callback(GLFWwindow *, int key, int scancode, int action, int mods);
        static void mouse_button_callback(GLFWwindow *, int button, int action, int mods);
        static void cursor_pos_callback(GLFWwindow *, double xpos, double ypos);
        static void handle_resize(GLFWwindow *, int width, int height);
    };
} // namespace engine
