#include "exceptions.hpp"
#include "vk_result_to_string.hpp"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

using namespace engine;

Exception::Exception(std::string message, std::source_location source)
    : m_message(std::move(message))
    , m_source_location(source)
    , std::runtime_error(message.data())
{ }

const std::source_location &Exception::get_source() const noexcept
{
    return m_source_location;
}

GlfwException::GlfwException(std::string message, std::source_location source)
    : Exception(message, source)
    , m_error_code(glfwGetError(&m_error_info))
{ }

int GlfwException::get_error_code() const noexcept
{
    return m_error_code;
}

std::string_view GlfwException::get_error_info() const noexcept
{
    return m_error_info;
}

engine::VulkanException::VulkanException(uint32_t result, std::string message, std::source_location source)
    : Exception(std::move(message), std::move(source))
    , m_result(result)
{ }

uint32_t engine::VulkanException::get_result() const noexcept
{
    return m_result;
}

std::string_view engine::VulkanException::get_error_string() const noexcept
{
    return result_to_string(m_result);
}

engine::VulkanExtensionsNotAvailable::VulkanExtensionsNotAvailable(std::string               message,
                                                                   std::vector<const char *> unavailable_extensions,
                                                                   std::source_location      source)
    : VulkanException((uint32_t)vk::Result::eErrorExtensionNotPresent, std::move(message), std::move(source))
    , m_unavailable_extensions(std::move(unavailable_extensions))
{ }

std::span<const char * const> engine::VulkanExtensionsNotAvailable::get_extensions() const noexcept
{
    return m_unavailable_extensions;
}

engine::VulkanLayersNotAvailable::VulkanLayersNotAvailable(std::string               message,
                                                           std::vector<const char *> unavailable_layers,
                                                           std::source_location      source)
    : VulkanException((uint32_t)vk::Result::eErrorLayerNotPresent, std::move(message), std::move(source))
    , m_unavailable_layers(std::move(unavailable_layers))
{ }

std::span<const char * const> engine::VulkanLayersNotAvailable::get_layers() const noexcept
{
    return m_unavailable_layers;
}
