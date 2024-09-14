#include "GouraudMesh.hpp"

namespace engine
{
    GouraudMesh::GouraudMesh(Allocation allocation, vk::DeviceSize vtx_off, vk::DeviceSize idx_off, uint32_t count)
        : allocation(std::move(allocation))
        , vtx_offset(vtx_off)
        , idx_offset(idx_off)
        , count(count)
    { }

    void GouraudMesh::draw(DrawingContext &context)
    {
        // context.cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, context.backend->m_gouraud_pipeline);
        context.cmd.bindVertexBuffers(0, allocation.buffer, vtx_offset);
        context.cmd.bindIndexBuffer(allocation.buffer, idx_offset, vk::IndexType::eUint32);
        context.cmd.drawIndexed(count, 1, 0, 0, 0);
    }

    GouraudMesh::~GouraudMesh() { }
} // namespace engine