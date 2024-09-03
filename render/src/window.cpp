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

static std::string_view get_type(vk::PhysicalDeviceType t)
{
    switch (t) {
    case vk::PhysicalDeviceType::eIntegratedGpu:
        return "Integrated";
    case vk::PhysicalDeviceType::eDiscreteGpu:
        return "Discrete";
    case vk::PhysicalDeviceType::eVirtualGpu:
        return "Virtual";
    default:
        return "Other";
    }
}

static void list_devices(shared_ptr<spdlog::logger>                                          logger,
                         span<const tuple<vk::PhysicalDevice, vk::PhysicalDeviceProperties>> devices)
{
    stringstream ss = {};

    for (auto &[device, properties] : devices)
        ss << fmt::format("\n\t[{}] {} ({} MB)", get_type(properties.deviceType), properties.deviceName.data(),
                          properties.limits.maxImageDimension2D);

    logger->info("Found {} supported devices:{}", devices.size(), ss.str());
}

static vector<tuple<vk::PhysicalDevice, vk::PhysicalDeviceProperties>> get_properties(
    span<const vk::PhysicalDevice> devices)
{
    vector<tuple<vk::PhysicalDevice, vk::PhysicalDeviceProperties>> pairs = {};
    pairs.reserve(devices.size());

    for (auto device : devices)
        pairs.push_back({device, device.getProperties()});

    return pairs;
}

static vk::PhysicalDevice select_physical_device(
    span<const tuple<vk::PhysicalDevice, vk::PhysicalDeviceProperties>> devices)
{
    if (devices.size() == 0)
        throw engine::Exception("No devices are available");

    vk::PhysicalDevice selected_device;
    uint64_t           selected_score = 0;

    for (auto &[device, properties] : devices) {
        uint64_t score = 0;
        score += properties.limits.maxImageDimension2D;
        score += (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) ? 100000 : 0;

        if (score > selected_score)
            selected_device = device, selected_score = score;
    }

    return selected_device;
}

namespace engine
{
    Window::Window(string_view title, int32_t width, int32_t height, string_view application_name,
                   Version application_version)
        : m_logger(get_logger())
    {
        if (!glfwInit())
            throw GlfwException("Failed to initialize GLFW");

        m_instance_manager = VulkanInstanceManager::new_shared(application_name, application_version);

        auto devices = get_properties(m_instance_manager->get_supported_rendering_devices());
        list_devices(m_instance_manager->m_logger, devices);
        auto device = select_physical_device(devices);

        m_device_manager = RenderDeviceManager::new_shared(m_instance_manager, device);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        mp_window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
        if (!mp_window)
            throw GlfwException("Failed to create a new window");
        m_logger->info("Created window");

        m_render_manager = RenderManager::new_unique(m_device_manager, mp_window);
    }

    Window::Window(string_view title, int32_t width, int32_t height, const Window &other)
        : m_logger(other.m_logger)
        , m_instance_manager(other.m_instance_manager)
        , m_device_manager(other.m_device_manager)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        mp_window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
        if (!mp_window)
            throw GlfwException("Failed to create a new window");
        m_logger->info("Created window");

        m_render_manager = RenderManager::new_unique(m_device_manager, mp_window);
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
        }
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
        , m_instance_manager(move(m_instance_manager))
        , m_device_manager(move(m_device_manager))
        , m_render_manager(move(m_render_manager))

    { }
    Window &Window::operator=(Window &&other) noexcept
    {
        swap(m_logger, other.m_logger);
        swap(m_instance_manager, other.m_instance_manager);
        swap(m_device_manager, other.m_device_manager);
        swap(m_render_manager, other.m_render_manager);

        return *this;
    }
} // namespace engine