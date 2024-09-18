#pragma once
#include "vertex.hpp"
#include <array>
#include <vulkan/vulkan.hpp>

#define VTX_BINDING(type, bind_index, rate)                                                                            \
    vk::VertexInputBindingDescription                                                                                  \
    {                                                                                                                  \
        .binding = bind_index, .stride = sizeof(type), .inputRate = vk::VertexInputRate::e##rate,                      \
    }
#define VTX_ATTRIBUTE(type, field, _format, _binding, _location)                                                       \
    vk::VertexInputAttributeDescription                                                                                \
    {                                                                                                                  \
        .location = _location, .binding = _binding, .format = vk::Format::e##_format, .offset = offsetof(type, field)  \
    }

namespace engine::primitives
{
    template<size_t B, size_t A>
    struct VertexDescription
    {
        std::array<vk::VertexInputBindingDescription, B>   bindings;
        std::array<vk::VertexInputAttributeDescription, A> attributes;
    };

    inline const VertexDescription<1, 2> GOURAUD_VERTEX {
        .bindings   = {VTX_BINDING(GouraudVertex, 0, Vertex)},
        .attributes = {VTX_ATTRIBUTE(GouraudVertex, position, R32G32B32Sfloat, 0, 0),
                       VTX_ATTRIBUTE(GouraudVertex, color, R32G32B32Sfloat, 0, 1)},
    };

    inline const VertexDescription<1, 2> TEXTURED_VERTEX {
        .bindings   = {VTX_BINDING(TexturedVertex, 0, Vertex)},
        .attributes = {VTX_ATTRIBUTE(TexturedVertex, position, R32G32B32Sfloat, 0, 0),
                       VTX_ATTRIBUTE(TexturedVertex, uv, R32G32Sfloat, 0, 1)},
    };
} // namespace engine::primitives

#undef VTX_BINDING
#undef VTX_ATTRIBUTE