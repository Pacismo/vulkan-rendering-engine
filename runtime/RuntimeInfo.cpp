#include "RuntimeInfo.hpp"
#include <fmt/format.h>
#include <imgui_stdlib.h>

RuntimeInfo::RuntimeInfo()
    : Applet("Runtime Information", false, true)
{ }

RuntimeInfo::~RuntimeInfo() { }

void RuntimeInfo::populate(ImGuiViewport *viewport)
{
    ImGui::Text("Engine: %s", ENGINE_NAME);
    ImGui::Text("Version: %u.%u.%u", ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_PATCH);

    if (DEBUG_ASSERTIONS)
        ImGui::Text("Debugging enabled");
}
