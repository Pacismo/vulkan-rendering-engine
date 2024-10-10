#pragma once
#include <gui/applet.hpp>
#include <transform.hpp>

class HintBox final : public engine::gui::Applet
{
  public:
    HintBox(engine::CameraTransform &camera_transform, float &fov, bool &show_demo_window, bool &show_cube_mutator,
            bool &camera_mouse);
    ~HintBox();

    void draw(ImGuiViewport *v = ImGui::GetMainViewport()) override;

  protected:
    void populate(ImGuiViewport *v) override;

  private:
    bool *show_demo_window;
    bool *show_cube_mutator;
    bool *camera_mouse;

    engine::CameraTransform *camera;
    float                   *fov;
};
