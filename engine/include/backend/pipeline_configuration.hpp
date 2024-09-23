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

        struct PreparedPipelineConfiguration prepare(vk::PipelineLayout pipeline_layout,
                                                     vk::RenderPass     render_pass) const;
    };

    struct PreparedPipelineConfiguration
    {
        PreparedPipelineConfiguration(const PipelineConfiguration &config, vk::PipelineLayout pipeline_layout, vk::RenderPass render_pass);

        vk::PipelineDynamicStateCreateInfo             dynamic_state;
        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;
        vk::PipelineVertexInputStateCreateInfo         vertex_input;
        vk::PipelineInputAssemblyStateCreateInfo       input_assembly;
        vk::PipelineViewportStateCreateInfo            viewport_state;
        vk::PipelineRasterizationStateCreateInfo       rasterizer;
        vk::PipelineMultisampleStateCreateInfo         multisampling;
        vk::PipelineDepthStencilStateCreateInfo        depth_stencil;
        vk::PipelineColorBlendStateCreateInfo          color_blending;
        vk::PipelineLayout                             pipeline_layout;
        vk::RenderPass                                 render_pass;

        operator vk::GraphicsPipelineCreateInfo() const;
    };

    inline PreparedPipelineConfiguration PipelineConfiguration::prepare(vk::PipelineLayout pipeline_layout,
                                                                        vk::RenderPass     render_pass) const
    {
        return PreparedPipelineConfiguration(*this, pipeline_layout, render_pass);
    }

    inline PreparedPipelineConfiguration::PreparedPipelineConfiguration(const PipelineConfiguration &config,
                                                                        vk::PipelineLayout           pipeline_layout,
                                                                        vk::RenderPass               render_pass)
        : rasterizer(config.rasterizer)
        , multisampling(config.multisampling)
        , pipeline_layout(pipeline_layout)
        , render_pass(render_pass)
    {
        dynamic_state = {
            .dynamicStateCount = (uint32_t)config.dynamic_states.size(),
            .pDynamicStates    = config.dynamic_states.data(),
        };

        shader_stages = {
            {
             .stage  = vk::ShaderStageFlagBits::eVertex,
             .module = config.vertex_shader,
             .pName  = "main",
             },
            {
             .stage  = vk::ShaderStageFlagBits::eFragment,
             .module = config.fragment_shader,
             .pName  = "main",
             },
        };

        vertex_input = {
            .vertexBindingDescriptionCount   = (uint32_t)config.vertex_binding_descriptions.size(),
            .pVertexBindingDescriptions      = config.vertex_binding_descriptions.data(),
            .vertexAttributeDescriptionCount = (uint32_t)config.vertex_attribute_descriptions.size(),
            .pVertexAttributeDescriptions    = config.vertex_attribute_descriptions.data(),
        };

        input_assembly = {
            .topology               = vk::PrimitiveTopology::eTriangleList,
            .primitiveRestartEnable = false,
        };

        viewport_state = {
            .viewportCount = 1,
            .scissorCount  = 1,
        };

        depth_stencil = {
            // TODO - later configuration
        };

        color_blending = {
            .logicOpEnable   = config.color_blending.logic_op_enabled,
            .logicOp         = config.color_blending.logic_op,
            .attachmentCount = (uint32_t)config.color_blending.attachments.size(),
            .pAttachments    = config.color_blending.attachments.data(),
            .blendConstants  = config.color_blending.constants,
        };
    }

    inline PreparedPipelineConfiguration::operator vk::GraphicsPipelineCreateInfo() const
    {
        return {
            .stageCount          = (uint32_t)shader_stages.size(),
            .pStages             = shader_stages.data(),
            .pVertexInputState   = &vertex_input,
            .pInputAssemblyState = &input_assembly,
            .pViewportState      = &viewport_state,
            .pRasterizationState = &rasterizer,
            .pMultisampleState   = &multisampling,
            .pDepthStencilState  = nullptr,
            .pColorBlendState    = &color_blending,
            .pDynamicState       = &dynamic_state,
            .layout              = pipeline_layout,
            .renderPass          = render_pass,
            .subpass             = 0,
            .basePipelineHandle  = nullptr,
            .basePipelineIndex   = -1,
        };
    }
} // namespace engine