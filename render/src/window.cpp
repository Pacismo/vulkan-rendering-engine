#include <vulkan/vulkan.hpp>

#include "device_manager.hpp"
#include "exceptions.hpp"
#include "instance_manager.hpp"
#include "logger.hpp"
#include "render_manager.hpp"
#include "window.hpp"
#include <algorithm>
#include <fmt/format.h>
#include <span>
#include <spdlog/spdlog.h>
#include <sstream>
#include <tuple>
#include <vector>

using std::string_view, std::span, std::vector, std::tuple, std::get, std::shared_ptr, std::stringstream, std::sort,
    std::move, std::swap;

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

        m_render_manager = RenderManager::new_unique(application_name, application_version, mp_window);
    }

    Window::Window(string_view title, int32_t width, int32_t height, const Window &other)
        : m_logger(other.m_logger)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        mp_window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
        if (!mp_window)
            throw GlfwException("Failed to create a new window");
        m_logger->info("Created window");

        m_render_manager = RenderManager::new_unique(other.m_render_manager, mp_window);
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

    void Window::run()
    {
        while (!glfwWindowShouldClose(mp_window)) {
            glfwPollEvents();
            process();
            m_render_manager->render_frame();
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