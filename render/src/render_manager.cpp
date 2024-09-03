#include "render_manager.hpp"
#include "exceptions.hpp"
#include <device_manager.hpp>
#include <instance_manager.hpp>

using std::move, std::swap;

namespace engine
{
    RenderManager::RenderManager(SharedDeviceManager device_manager, GLFWwindow *window)
        : m_logger(device_manager->m_logger)
        , m_instance_manager(device_manager->m_instance_manager)
        , m_device_manager(std::move(device_manager))
    {
        {
            VkSurfaceKHR surface = nullptr;
            VkResult     result  = glfwCreateWindowSurface(m_instance_manager->m_instance, window, nullptr, &surface);

            if (result != VK_SUCCESS)
                throw VulkanException(result, "Failed to create a GLFW window surface");

            m_logger->info("Created surface");

            m_surface = surface;
        }
    }

    RenderManager::~RenderManager()
    {
        if (m_surface)
            m_instance_manager->m_instance.destroySurfaceKHR(m_surface);

        m_logger->info("Destroyed render manager");

        m_surface          = nullptr;
        m_device_manager   = nullptr;
        m_instance_manager = nullptr;
    }
    RenderManager::Unique RenderManager::new_unique(SharedDeviceManager device_manager, GLFWwindow *window)
    {
        return Unique(new RenderManager(device_manager, window));
    }
    RenderManager::RenderManager(RenderManager &&other) noexcept
        : m_logger(move(other.m_logger))
        , m_instance_manager(move(other.m_instance_manager))
        , m_device_manager(move(other.m_device_manager))
        , m_surface(move(other.m_surface))
    { }

    RenderManager &RenderManager::operator=(RenderManager &&other) noexcept
    {
        swap(m_logger, other.m_logger);
        swap(m_instance_manager, other.m_instance_manager);
        swap(m_device_manager, other.m_device_manager);
        swap(m_surface, other.m_surface);

        return *this;
    }
} // namespace engine