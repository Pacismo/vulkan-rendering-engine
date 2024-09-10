#pragma once
#include "vertex.hpp"
#include <initializer_list>
#include <vulkan/vulkan.hpp>

namespace engine::primitives
{
    inline constexpr vk::VertexInputBindingDescription VERTEX_BINDING_DESCRIPTION = {
        .binding   = 0,
        .stride    = sizeof(Vertex),
        .inputRate = vk::VertexInputRate::eVertex,
    };

    inline constexpr std::initializer_list<vk::VertexInputAttributeDescription> VERTEX_INPUT_DESCRIPTION = {
        {.location = 0, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, position)},
        {.location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex,    color)},
    };
} // namespace engine::primitives