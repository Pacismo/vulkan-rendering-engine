#include "exceptions.hpp"
#include "vk_result.hpp"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <vulkan/vulkan.hpp>

namespace engine
{

    Exception::Exception(std::string message, std::source_location source)
        : m_message(std::move(message))
        , m_source_location(source)
        , std::runtime_error(message.data())
    { }

    const std::source_location &Exception::get_source() const noexcept
    {
        return m_source_location;
    }

    void Exception::log() const
    {
        spdlog::error("{}", m_message);
    }

    GlfwException::GlfwException(std::string message, std::source_location source)
        : Exception(message, source)
        , m_error_code(glfwGetError(&m_error_info))
    { }

    GlfwException::GlfwException(std::string message, int glfw_error_code, const char *glfw_error_msg,
                                 std::source_location source)

        : Exception(std::move(message), std::move(source))
        , m_error_code(glfw_error_code)
        , m_error_info(glfw_error_msg)
    { }

    int GlfwException::get_error_code() const noexcept
    {
        return m_error_code;
    }

    std::string_view GlfwException::get_error_info() const noexcept
    {
        return m_error_info;
    }

    void GlfwException::log() const
    {
        spdlog::error("GLFW Exception ({}): {}\n{}", m_error_code, m_message, m_error_info);
    }

    VulkanException::VulkanException(uint32_t result, std::string message, std::source_location source)
        : Exception(std::move(message), std::move(source))
        , m_result(result)
    { }

    uint32_t VulkanException::get_result() const noexcept
    {
        return m_result;
    }

    std::string_view VulkanException::get_error_string() const noexcept
    {
        return result_to_string(m_result);
    }

    void engine::VulkanException::log() const
    {
        spdlog::error("Vulkan Exception ([{}] {}): {}", m_result, get_error_string(), m_message);
    }

    VulkanExtensionsNotAvailable::VulkanExtensionsNotAvailable(std::string               message,
                                                               std::vector<const char *> unavailable_extensions,
                                                               std::source_location      source)
        : VulkanException((uint32_t)vk::Result::eErrorExtensionNotPresent, std::move(message), std::move(source))
        , m_unavailable_extensions(std::move(unavailable_extensions))
    { }

    std::span<const char *const> VulkanExtensionsNotAvailable::get_extensions() const noexcept
    {
        return m_unavailable_extensions;
    }

    void VulkanExtensionsNotAvailable::log() const
    {
        std::stringstream list = {};

        for (auto extension : m_unavailable_extensions)
            list << fmt::format("\n\t- {}", extension);

        spdlog::error("Vulkan Extensions Unavailable: {}{}", m_message, list.str());
    }

    VulkanLayersNotAvailable::VulkanLayersNotAvailable(std::string               message,
                                                       std::vector<const char *> unavailable_layers,
                                                       std::source_location      source)
        : VulkanException((uint32_t)vk::Result::eErrorLayerNotPresent, std::move(message), std::move(source))
        , m_unavailable_layers(std::move(unavailable_layers))
    { }

    std::span<const char *const> VulkanLayersNotAvailable::get_layers() const noexcept
    {
        return m_unavailable_layers;
    }

    void VulkanLayersNotAvailable::log() const
    {
        std::stringstream list = {};

        for (auto layer : m_unavailable_layers)
            list << fmt::format("\n\t- {}", layer);

        spdlog::error("Vulkan Layers Unavailable: {}{}", m_message, list.str());
    }
} // namespace engine
