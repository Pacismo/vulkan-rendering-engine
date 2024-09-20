#include <vulkan/vulkan.hpp>

#include "backend/device_manager.hpp"
#include "backend/instance_manager.hpp"
#include "backend/vulkan_backend.hpp"
#include "drawables/drawing_context.hpp"
#include "exceptions.hpp"
#include "logger.hpp"
#include "window.hpp"
#include <algorithm>
#include <chrono>
#include <fmt/format.h>
#include <span>
#include <spdlog/spdlog.h>
#include <sstream>
#include <tuple>
#include <vector>

using std::string_view, std::span, std::vector, std::tuple, std::get, std::shared_ptr, std::stringstream, std::sort,
    std::move, std::swap, std::optional;
using std::chrono::system_clock, std::chrono::time_point, std::chrono::duration, std::chrono::duration_cast,
    std::chrono::time_point_cast, std::chrono::nanoseconds, std::chrono::seconds;
using namespace std::chrono_literals;

namespace engine
{
    Window::Window(string_view title, int32_t width, int32_t height, string_view application_name,
                   Version application_version)
        : m_logger(get_logger())
    {
        if (!glfwInit())
            throw GlfwException("Failed to initialize GLFW");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
        if (!m_window)
            throw GlfwException("Failed to create a new window");
        m_logger->info("Created window");

        set_glfw_callbacks();

        m_render_manager = VulkanBackend::new_shared(application_name, application_version, m_window);
    }

    Window::Window(string_view title, int32_t width, int32_t height, const Window &other)
        : m_logger(other.m_logger)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
        if (!m_window)
            throw GlfwException("Failed to create a new window");
        m_logger->info("Created window");

        set_glfw_callbacks();

        m_render_manager = VulkanBackend::new_shared(other.m_render_manager, m_window);
    }

    void Window::show()
    {
        glfwShowWindow(m_window);
    }

    void Window::hide()
    {
        glfwHideWindow(m_window);
    }

    glm::vec2 Window::get_axis(KeyboardKey px, KeyboardKey nx, KeyboardKey py, KeyboardKey ny)
    {
        glm::vec2 axis(get_magnitude(px, nx), get_magnitude(py, ny));

        if (glm::abs(axis.x) + glm::abs(axis.y) > FLT_EPSILON)
            return glm::normalize(axis);
        else
            return {0.0, 0.0};
    }

    float Window::get_magnitude(KeyboardKey p, KeyboardKey n)
    {
        return glfwGetKey(m_window, (int)p) - glfwGetKey(m_window, (int)n);
    }

    void Window::close(bool should_close)
    {
        glfwSetWindowShouldClose(m_window, should_close);
    }

    void Window::set_title(string_view new_title)
    {
        glfwSetWindowTitle(m_window, new_title.data());
    }

    Window::SharedRenderManager Window::get_render_backend()
    {
        return m_render_manager;
    }

    void Window::on_key_action(KeyboardKey key, ModifierKey modifiers, KeyAction action, int scancode) { }

    void Window::on_mouse_button_action(MouseButton button, ModifierKey modifiers, KeyAction action) { }

    void Window::on_cursor_motion(double x, double y, double dx, double dy) { }

    void Window::process(double delta) { }

    void Window::physics_process(double delta) { }

    void Window::run(double pproc_freq)
    {
        double     avg            = 0.0;
        uint32_t   count          = 0;
        uint32_t   max            = 5;
        duration   print_period   = 1s;
        time_point last_draw      = system_clock::now();
        time_point next_print     = last_draw;
        time_point next_physics   = last_draw;
        duration   physics_period = duration<double, seconds::period>(1.0 / pproc_freq);

        try {
            while (!glfwWindowShouldClose(m_window)) {
                time_point now          = system_clock::now();
                duration   render_delta = duration_cast<duration<double>>(now - last_draw);

                glfwPollEvents();
                process(render_delta.count());

                if (now >= next_physics) {
                    physics_process(physics_period.count());
                    next_physics = next_physics + duration_cast<system_clock::duration>(physics_period);
                }

                time_point start = system_clock::now();

                if (optional<DrawingContext> ctx = m_render_manager->begin_draw()) {
                    handle_draw(ctx.value());
                    m_render_manager->end_draw(ctx.value());
                }

                duration dur = system_clock::now() - start;
                last_draw    = now;

                if (count < max)
                    count += 1;
                avg = (avg * (count - 1) + render_delta.count()) / count;
                if (now >= next_print) {
                    spdlog::info("Average FPS: {:.2F}", 1.0 / avg);
                    next_print += print_period;
                }
            }
        } catch (vk::SystemError &error) {
            auto &code    = error.code();
            auto  message = error.what();
            m_logger->error("Error encountered when calling {}", message);
            throw VulkanException(code.value(), message);
        }

        m_render_manager->wait_idle();
    }

    Window::~Window()
    {
        m_render_manager = nullptr;

        if (m_window)
            glfwDestroyWindow(m_window);
        m_window = nullptr;
    }

    Window::Window(Window &&other) noexcept
        : m_logger(move(m_logger))
        , m_render_manager(move(m_render_manager))

    { }

    Window &Window::operator=(Window &&other) noexcept
    {
        swap(m_logger, other.m_logger);
        swap(m_render_manager, other.m_render_manager);

        return *this;
    }

    void Window::set_glfw_callbacks()
    {
        glfwSetWindowUserPointer(m_window, this);

        // Force a poll and get the initial mouse coordinates (to avoid instant view snapping)
        glfwPollEvents();
        glfwGetCursorPos(m_window, &last_mouse_x, &last_mouse_y);

        glfwSetKeyCallback(m_window, key_callback);
        glfwSetMouseButtonCallback(m_window, mouse_button_callback);
        glfwSetCursorPosCallback(m_window, cursor_pos_callback);
    }

    void Window::key_callback(GLFWwindow *p_wnd, int key, int action, int scancode, int mods)
    {
        Window *window = (Window *)glfwGetWindowUserPointer(p_wnd);
        window->on_key_action(KeyboardKey(key), ModifierKey(mods), KeyAction(action), scancode);
    }

    void Window::mouse_button_callback(GLFWwindow *p_wnd, int button, int action, int mods)
    {
        Window *window = (Window *)glfwGetWindowUserPointer(p_wnd);
        window->on_mouse_button_action(MouseButton(button), ModifierKey(mods), KeyAction(action));
    }

    void Window::cursor_pos_callback(GLFWwindow *p_wnd, double xpos, double ypos)
    {
        Window *window       = (Window *)glfwGetWindowUserPointer(p_wnd);
        double  dx           = xpos - window->last_mouse_x;
        double  dy           = ypos - window->last_mouse_y;
        window->last_mouse_x = xpos;
        window->last_mouse_y = ypos;
        window->on_cursor_motion(xpos, ypos, dx, dy);
    }
} // namespace engine