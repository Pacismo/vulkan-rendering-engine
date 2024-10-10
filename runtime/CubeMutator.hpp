#pragma once
#include <gui/applet.hpp>
#include "Cube.hpp"
#include <window.hpp>

class CubeMutator final : public engine::gui::Applet
{
  public:
    CubeMutator(std::shared_ptr<Cube> cube = nullptr);
    ~CubeMutator();

    void set_cube(std::shared_ptr<Cube> cube);

    protected:
    void populate(ImGuiViewport *viewport) override;

    private:
    std::shared_ptr<Cube> m_cube;
};