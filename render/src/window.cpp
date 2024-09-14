#include <vulkan/vulkan.hpp>

#include "backend/device_manager.hpp"
#include "backend/instance_manager.hpp"
#include "backend/vulkan_backend.hpp"
#include "drawables/DrawingContext.hpp"
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
        mp_window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
        if (!mp_window)
            throw GlfwException("Failed to create a new window");
        glfwSetWindowUserPointer(mp_window, this);
        m_logger->info("Created window");

        m_render_manager = VulkanBackend::new_shared(application_name, application_version, mp_window);
    }

    Window::Window(string_view title, int32_t width, int32_t height, const Window &other)
        : m_logger(other.m_logger)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        mp_window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
        if (!mp_window)
            throw GlfwException("Failed to create a new window");
        m_logger->info("Created window");

        m_render_manager = VulkanBackend::new_shared(other.m_render_manager, mp_window);
    }

    void Window::show()
    {
        glfwShowWindow(mp_window);
    }

    void Window::hide()
    {
        glfwHideWindow(mp_window);
    }

    void Window::set_title(string_view new_title)
    {
        glfwSetWindowTitle(mp_window, new_title.data());
    }

    std::shared_ptr<RenderBackend> Window::get_render_backend()
    {
        return m_render_manager;
    }

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
        duration   physics_period = duration_cast<system_clock::duration>(duration<double>(1.0 / pproc_freq));

        try {
            while (!glfwWindowShouldClose(mp_window)) {
                time_point now          = system_clock::now();
                duration   render_delta = duration_cast<duration<double>>(now - last_draw);

                glfwPollEvents();
                process(render_delta.count());

                if (now >= next_physics) {
                    physics_process(physics_period.count());
                    next_physics = next_physics + physics_period;
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

        if (mp_window)
            glfwDestroyWindow(mp_window);
        mp_window = nullptr;
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
} // namespace engine