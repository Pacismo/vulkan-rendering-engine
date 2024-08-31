#pragma once
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <span>

namespace engine
{
    /// Base exception type for use in the engine
    ///
    /// Has provisions for getting information about the source of an error
    class Exception : public std::runtime_error
    {
      public:
        Exception(std::string message, std::source_location source = std::source_location::current());
        virtual ~Exception() = default;

        /// Get the source of the error
        const std::source_location &get_source() const noexcept;

      private:
        std::string          m_message;
        std::source_location m_source_location;
    };

    /// Exception class for all GLFW exceptions
    class GlfwException final : public Exception
    {
      public:
        GlfwException(std::string message, std::source_location source = std::source_location::current());

        ~GlfwException() = default;

        /// Get the GLFW error code
        int              get_error_code() const noexcept;
        /// Get the string description of the error code
        std::string_view get_error_info() const noexcept;

      private:
        int         m_error_code;
        const char *m_error_info;
    };

    /// Base exception class for all Vulkan exceptions
    class VulkanException : public Exception
    {
      public:
        VulkanException(uint32_t result, std::string message,
                        std::source_location source = std::source_location::current());

        ~VulkanException() = default;

        /// Get the integer representation of the result
        uint32_t         get_result() const noexcept;
        /// Get the string representation of the result
        std::string_view get_error_string() const noexcept;

      private:
        uint32_t m_result;
    };

    /// Required extensions are not available
    class VulkanExtensionsNotAvailable final : public VulkanException
    {
      public:
        VulkanExtensionsNotAvailable(std::string message, std::vector<const char *> unavailable_extensions,
                                     std::source_location source = std::source_location::current());

        ~VulkanExtensionsNotAvailable() = default;

        /// Get a span over the required extensions
        std::span<const char * const> get_extensions() const noexcept;

      private:
        std::vector<const char *> m_unavailable_extensions;
    };

    /// Required layers are not available
    class VulkanLayersNotAvailable final : public VulkanException
    {
      public:
        VulkanLayersNotAvailable(std::string message, std::vector<const char *> unavailable_layers,
                                 std::source_location source = std::source_location::current());

        ~VulkanLayersNotAvailable() = default;

        /// Get a span over the required layers
        std::span<const char * const> get_layers() const noexcept;

      private:
        std::vector<const char *> m_unavailable_layers;
    };
} // namespace engine
