#pragma once
#include <vulkan/vulkan.hpp>

namespace engine
{
    struct ColorBlending
    {
        using Attachment = vk::PipelineColorBlendAttachmentState;

        bool                    logic_op_enabled = false;
        vk::LogicOp             logic_op         = {};
        std::vector<Attachment> attachments      = {};
        std::array<float, 4>    constants        = {};
    };

    struct PipelineConfiguration
    {
        using VertexBindingDescriptions   = std::vector<vk::VertexInputBindingDescription>;
        using VertexAttributeDescriptions = std::vector<vk::VertexInputAttributeDescription>;
        using DynamicStates               = std::vector<vk::DynamicState>;
        using RasterizerConfiguration     = vk::PipelineRasterizationStateCreateInfo;
        using MultisampleConfiguration    = vk::PipelineMultisampleStateCreateInfo;

        vk::ShaderModule            vertex_shader                 = {};
        vk::ShaderModule            fragment_shader               = {};
        VertexBindingDescriptions   vertex_binding_descriptions   = {};
        VertexAttributeDescriptions vertex_attribute_descriptions = {};
        DynamicStates               dynamic_states                = {};
        RasterizerConfiguration     rasterizer                    = {};
        MultisampleConfiguration    multisampling                 = {};
        ColorBlending               color_blending                = {};
    };
} // namespace engine