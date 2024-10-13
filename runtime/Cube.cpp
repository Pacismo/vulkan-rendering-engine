#include "Cube.hpp"
#include <array>
#include <backend/vulkan_backend.hpp>
#include <drawables/GouraudMesh.hpp>
#include <vertex.hpp>

using engine::primitives::GouraudVertex;
using namespace engine::reflection;

static const std::array<GouraudVertex, 24> VERTICES = {
  // -Z
    GouraudVertex {.position = {-0.5, -0.5, -0.5}, .color = {0.0, 0.0, 0.5}},
    GouraudVertex { .position = {0.5, -0.5, -0.5}, .color = {0.0, 0.0, 0.5}},
    GouraudVertex {  .position = {0.5, 0.5, -0.5}, .color = {0.0, 0.0, 0.5}},
    GouraudVertex { .position = {-0.5, 0.5, -0.5}, .color = {0.0, 0.0, 0.5}},

 // +Z
    GouraudVertex { .position = {-0.5, -0.5, 0.5}, .color = {0.0, 0.0, 1.0}},
    GouraudVertex {  .position = {0.5, -0.5, 0.5}, .color = {0.0, 0.0, 1.0}},
    GouraudVertex {   .position = {0.5, 0.5, 0.5}, .color = {0.0, 0.0, 1.0}},
    GouraudVertex {  .position = {-0.5, 0.5, 0.5}, .color = {0.0, 0.0, 1.0}},

 // -Y
    GouraudVertex {.position = {-0.5, -0.5, -0.5}, .color = {0.0, 0.5, 0.0}},
    GouraudVertex { .position = {0.5, -0.5, -0.5}, .color = {0.0, 0.5, 0.0}},
    GouraudVertex { .position = {-0.5, -0.5, 0.5}, .color = {0.0, 0.5, 0.0}},
    GouraudVertex {  .position = {0.5, -0.5, 0.5}, .color = {0.0, 0.5, 0.0}},

 // +Y
    GouraudVertex {  .position = {0.5, 0.5, -0.5}, .color = {0.0, 1.0, 0.0}},
    GouraudVertex { .position = {-0.5, 0.5, -0.5}, .color = {0.0, 1.0, 0.0}},
    GouraudVertex {   .position = {0.5, 0.5, 0.5}, .color = {0.0, 1.0, 0.0}},
    GouraudVertex {  .position = {-0.5, 0.5, 0.5}, .color = {0.0, 1.0, 0.0}},

 // -X
    GouraudVertex { .position = {-0.5, 0.5, -0.5}, .color = {0.5, 0.0, 0.0}},
    GouraudVertex { .position = {-0.5, -0.5, 0.5}, .color = {0.5, 0.0, 0.0}},
    GouraudVertex {  .position = {-0.5, 0.5, 0.5}, .color = {0.5, 0.0, 0.0}},
    GouraudVertex {.position = {-0.5, -0.5, -0.5}, .color = {0.5, 0.0, 0.0}},

 // +X
    GouraudVertex { .position = {0.5, -0.5, -0.5}, .color = {1.0, 0.0, 0.0}},
    GouraudVertex {  .position = {0.5, 0.5, -0.5}, .color = {1.0, 0.0, 0.0}},
    GouraudVertex {  .position = {0.5, -0.5, 0.5}, .color = {1.0, 0.0, 0.0}},
    GouraudVertex {   .position = {0.5, 0.5, 0.5}, .color = {1.0, 0.0, 0.0}},
};

static const std::array<uint32_t, 36> INDICES = {
    0,  1,  2,  2,  3,  0,  //  0  1  2  3
    6,  5,  4,  4,  7,  6,  //  4  5  6  7
    11, 9,  8,  10, 11, 8,  //  8  9 10 11
    15, 13, 12, 14, 15, 12, // 12 13 14 15
    17, 19, 16, 18, 17, 16, // 16 17 18 19
    23, 21, 20, 22, 23, 20, // 20 21 22 23
};

Cube::Cube(engine::VulkanBackend &backend)
{
    auto vertices = VERTICES;
    auto indices  = INDICES;

    mesh = backend.load(vertices, indices);
}

void Cube::physics_process(double delta)
{
    if (rotate)
        transform.rotation.z = glm::mod<float>(transform.rotation.z + 180.0_deg * delta, 360.0_deg);
}

void Cube::draw(engine::DrawingContext &context, const glm::mat4 &parent)
{
    mesh->draw(context, parent * glm::mat4(transform));
}

const Field CUBE_FIELDS[] = {
    Field("rotate", FieldTypeBits::Boolean, offsetof(Cube, rotate)),
};

const Datastructure CUBE_REP = Datastructure("Cube", CUBE_FIELDS, &engine::OBJECT_REP);

const engine::reflection::Datastructure *Cube::get_rep() const
{
    return &CUBE_REP;
}
