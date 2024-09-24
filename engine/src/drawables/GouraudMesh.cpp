#include "drawables/GouraudMesh.hpp"
#include "backend/vulkan_backend.hpp"
#include "drawables/drawing_context.hpp"
#include <array>

using std::array;

namespace engine
{
    GouraudMesh::GouraudMesh(BufferAllocation allocation, vk::DeviceSize vtx_off, vk::DeviceSize idx_off, uint32_t count)
        : allocation(std::move(allocation))
        , model_matrix(this->allocation.allocator, vk::BufferUsageFlagBits::eUniformBuffer)
        , vtx_offset(vtx_off)
        , idx_offset(idx_off)
        , count(count)
    { }

    void GouraudMesh::draw(DrawingContext &context, const glm::mat4 &parent_transform)
    {
        model_matrix[context.frame_index] = parent_transform * transform.get_transform_matrix();
        model_matrix.flush();

        vk::DescriptorSet descriptor = context.descriptors[context.used_descriptors++];

        array<vk::DescriptorBufferInfo, 2> dbi = {
            context.vp_buffer_info,
            vk::DescriptorBufferInfo {.buffer = model_matrix.buffer,
                                      .offset = model_matrix.offset(context.frame_index),
                                      .range  = model_matrix.type_size()},
        };

        vk::WriteDescriptorSet wds = {
            .dstSet           = descriptor,
            .dstBinding       = 0,
            .dstArrayElement  = 0,
            .descriptorCount  = dbi.size(),
            .descriptorType   = vk::DescriptorType::eUniformBuffer,
            .pImageInfo       = nullptr,
            .pBufferInfo      = dbi.data(),
            .pTexelBufferView = nullptr,
        };

        context.backend->m_device.updateDescriptorSets(wds, {});

        // context.cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, context.backend->m_gouraud_pipeline);
        context.cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, context.backend->m_pipeline_layout, 0,
                                       descriptor, {});
        context.cmd.bindVertexBuffers(0, allocation.buffer, vtx_offset);
        context.cmd.bindIndexBuffer(allocation.buffer, idx_offset, vk::IndexType::eUint32);
        context.cmd.drawIndexed(count, 1, 0, 0, 0);
    }

    GouraudMesh::~GouraudMesh() { }
} // namespace engine
