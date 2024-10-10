#include "CubeMutator.hpp"

CubeMutator::CubeMutator(std::shared_ptr<Cube> cube)
    : Applet("Cube Mutator", false, true)
    , m_cube(cube)
{ }

CubeMutator::~CubeMutator() = default;

void CubeMutator::set_cube(std::shared_ptr<Cube> cube)
{
    m_cube = cube;
}

void CubeMutator::populate(ImGuiViewport *viewport) { 
    if (!m_cube)
        return;

    ImGui::Checkbox("Enable Rotation", &m_cube->rotate);

    ImGui::DragFloat3("Location", &m_cube->transform.location.x, 1.0);
    ImGui::DragFloat3("Rotation", &m_cube->transform.rotation.x, 0.1, 0.0, 360.0_deg, "%.3f",
                      ImGuiSliderFlags_WrapAround);
    ImGui::DragFloat3("Scale", &m_cube->transform.scale.x, 1.0);
}
