#pragma once
#include <backend/vulkan_backend.hpp>
#include <drawables/GouraudMesh.hpp>
#include <object.hpp>

class Cube : public engine::Object
{
  public:
    Cube(engine::VulkanBackend &backend);

    void physics_process(double delta) override;

    void draw(engine::DrawingContext &context, const glm::mat4 &parent) override;

    std::shared_ptr<engine::GouraudMesh> mesh;
    bool                                 rotate = true;
};