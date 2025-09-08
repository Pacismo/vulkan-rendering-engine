#pragma once
#include <gui/applet.hpp>
#include <object.hpp>
#include <window.hpp>

class RuntimeInfo final : public engine::gui::Applet
{
  public:
    RuntimeInfo();
    ~RuntimeInfo();

  protected:
    void populate(ImGuiViewport *viewport) override;
};
