#include "HintBox.hpp"
#include <fmt/format.h>

static ImGuiWindowFlags DEFAULT_FLAGS = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize
                                      | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing
                                      | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

static constexpr const char *STRINGS[] = {
    "F1  - Debug Picker",
    "F2  - Show Cube Mutator Window",
    "F3  - Show ImGui Demo Window",
    "TAB - Enable/Disable Mouse Capture",
    "R   - Reset World",
};

HintBox::HintBox(engine::CameraTransform &camera_transform, float &fov, bool &show_demo_window, bool &show_cube_mutator,
                 bool &camera_mouse)
    : Applet("Hints", true, false, DEFAULT_FLAGS)
    , camera(&camera_transform)
    , fov(&fov)
    , show_demo_window(&show_demo_window)
    , show_cube_mutator(&show_cube_mutator)
    , camera_mouse(&camera_mouse)
{ }

HintBox::~HintBox() = default;

void HintBox::draw(ImGuiViewport *viewport)
{
    constexpr float PADDING = 10.0f;

    auto     pos           = viewport->WorkPos;
    auto     size          = viewport->WorkSize;
    ImVec2   overlay_pos   = ImVec2(pos.x + size.x - PADDING, pos.y + size.y - PADDING);
    ImVec2   overlay_pivot = ImVec2(1.0, 1.0);
    ImGui::SetNextWindowPos(overlay_pos, ImGuiCond_Always, overlay_pivot);

    Applet::draw(viewport);
}

void HintBox::populate(ImGuiViewport *viewport)
{
    assert(show_demo_window && show_cube_mutator && camera_mouse && camera && fov);

    ImGui::Text("Shortcuts");
    const ImVec4 on_color  = ImVec4(0.0, 1.0, 0.0, 1.0);
    const ImVec4 off_color = ImVec4(1.0, 0.0, 0.0, 1.0);

#ifndef NDEBUG
    ImGui::Text(STRINGS[0]);
#endif

    if (*show_cube_mutator)
        ImGui::TextColored(on_color, STRINGS[1]);
    else
        ImGui::TextColored(off_color, STRINGS[1]);

    if (*show_demo_window)
        ImGui::TextColored(on_color, STRINGS[2]);
    else
        ImGui::TextColored(off_color, STRINGS[2]);

    if (*camera_mouse)
        ImGui::TextColored(on_color, STRINGS[3]);
    else
        ImGui::TextColored(off_color, STRINGS[3]);

    ImGui::Text(STRINGS[4]);

    ImGui::Separator();

    std::string str = fmt::format("Location: [{: 5.3F}, {: 5.3F}, {: 5.3F}]", camera->location.x, camera->location.y,
                                  camera->location.z);
    ImGui::TextUnformatted(str.c_str());

    str = fmt::format("Rotation: [{:>7.3F}, {:>7.3F}]", glm::degrees(camera->rotation.x),
                      glm::degrees(camera->rotation.y));
    ImGui::TextUnformatted(str.c_str());

    str = fmt::format("FOV:      {:.1F}", *fov);
    ImGui::TextUnformatted(str.c_str());
}