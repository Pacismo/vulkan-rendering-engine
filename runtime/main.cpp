#include <backend/vulkan_backend.hpp>
#include <exceptions.hpp>
#include <fmt/format.h>
#include <glfw/glfw3.h>
#include <imgui.h>
#include <memory>
#include <object.hpp>
#include <span>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <vertex.hpp>
#include <vulkan/vulkan.hpp>
#include <window.hpp>

#if TERMINAL_ENABLED or not defined(WIN32)
#    define MAIN int main()
#else
#    define MAIN int WinMain()
#endif

using namespace std::chrono_literals;
using engine::primitives::GouraudVertex, engine::Object, engine::CameraTransform, engine::KeyboardKey,
    engine::KeyAction, engine::ModifierKey, engine::contains, engine::DEFAULT_FOV;
using std::shared_ptr, std::initializer_list, engine::Window, std::string_view, std::array, std::list, std::shared_ptr;

const array<GouraudVertex, 24> VERTICES = {
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

const array<uint32_t, 36> INDICES = {
    0,  1,  2,  2,  3,  0,  //  0  1  2  3
    6,  5,  4,  4,  7,  6,  //  4  5  6  7
    11, 9,  8,  10, 11, 8,  //  8  9 10 11
    15, 13, 12, 14, 15, 12, // 12 13 14 15
    17, 19, 16, 18, 17, 16, // 16 17 18 19
    23, 21, 20, 22, 23, 20, // 20 21 22 23
};

class Cube : public Object
{
  public:
    Cube(engine::VulkanBackend &backend)
    {
        auto vertices = VERTICES;
        auto indices  = INDICES;

        mesh = backend.load(vertices, indices);
    }

    void physics_process(double delta) override
    {
        if (rotate)
            transform.rotation.z = glm::mod<float>(transform.rotation.z + 180.0_deg * delta, 360.0_deg);
    }

    void draw(engine::DrawingContext &context, const glm::mat4 &) override { mesh->draw(context, transform); }

    std::shared_ptr<Object> mesh;
    bool                    rotate = true;
};

class ExampleWindow : public Window
{
  public:
    void capture_mouse(bool capture)
    {
        if (capture && glfwRawMouseMotionSupported()) {
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        } else {
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    ExampleWindow(string_view title, int width, int height)
        : Window(title, width, height, "Runtime", {0, 1, 0})
    {
        auto &rb = get_render_backend();

        cube = shared_ptr<Cube>(new Cube(rb));
        cube_2 = shared_ptr<Cube>(new Cube(rb));
        objects.push_back(cube);
        objects.push_back(cube_2);

        camera.location = {2.0, 2.0, 2.0};
        camera.rotation = {135.0_deg, -35.0_deg};

        capture_mouse(camera_mouse);

        rb.update_view(camera);
    }

    void on_key_action(KeyboardKey key, ModifierKey mods, KeyAction action, int scancode) override
    {
        using KeyboardKey::Tab, KeyboardKey::Escape, KeyboardKey::F1, KeyboardKey::F2, KeyboardKey::R;

        switch (key) {
        case Escape:
            close();
            break;
        case Tab:
            if (action == KeyAction::Press)
                capture_mouse(camera_mouse = !camera_mouse);
            break;
        case F1:
            if (action == KeyAction::Press)
                show_demo_window = !show_demo_window;
            break;
        case F2:
            if (action == KeyAction::Press)
                show_cube_mutator = !show_cube_mutator;
            break;
        case R:
            if (action == KeyAction::Press) {
                camera.location = {2.0, 2.0, 2.0};
                camera.rotation = {135.0_deg, -35.0_deg};
                get_render_backend().update_fov(fov = DEFAULT_FOV);

                cube->transform.location = {0.0, 0.0, 0.0};
                cube->transform.rotation = {0.0, 0.0, 0.0};
                cube->transform.scale    = {1.0, 1.0, 1.0};
            }
            break;
        default:
            break;
        }
    }

    void on_cursor_motion(double x, double y, double dx, double dy) override
    {
        constexpr glm::vec2 COEFFICIENT = {-0.01, -0.01};

        if (!camera_mouse)
            return;

        camera.rotation.x = glm::mod(camera.rotation.x + dx * COEFFICIENT.x, 360.0_deg);
        camera.rotation.y = glm::clamp(camera.rotation.y + dy * COEFFICIENT.y, -89.9_deg, 89.9_deg);
    }

    void on_scroll(double xoff, double yoff) override
    {
        constexpr float COEFFICIENT = -5.0;

        if (!camera_mouse)
            return;

        fov = glm::clamp(fov + yoff * COEFFICIENT, 15.0, 100.0);
        get_render_backend().update_fov(fov);
    }

    static constexpr float MOTION_SPEED = 2.5;

    bool show_demo_window  = false;
    bool show_cube_mutator = false;

    void cube_mutator()
    {
        ImGuiIO         &io    = ImGui::GetIO();
        ImGuiWindowFlags flags = ImGuiWindowFlags_None;

        if (ImGui::Begin("Cube Mutator", &show_cube_mutator, flags)) {
            ImGui::Checkbox("Enable Rotation", &cube->rotate);

            ImGui::DragFloat3("Location", &cube->transform.location.x, 1.0);
            ImGui::DragFloat3("Rotation", &cube->transform.rotation.x, 0.1, 0.0, 360.0_deg, "%.3f",
                              ImGuiSliderFlags_WrapAround);
            ImGui::DragFloat3("Scale", &cube->transform.scale.x, 1.0);
        }
        ImGui::End();
    }

    void draw_hint_box()
    {
        static constexpr const char *strings[] = {
            "F1  - Show ImGui Demo Window",
            "F2  - Show Cube Mutator Window",
            "TAB - Enable/Disable Mouse Capture",
            "R   - Reset World",
        };

        constexpr float  PADDING = 10.0f;
        ImGuiIO         &io      = ImGui::GetIO();
        ImGuiWindowFlags flags   = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize
                               | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing
                               | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

        auto   viewport      = ImGui::GetMainViewport();
        auto   pos           = viewport->WorkPos;
        auto   size          = viewport->WorkSize;
        ImVec2 overlay_pos   = ImVec2(pos.x + size.x - PADDING, pos.y + size.y - PADDING);
        ImVec2 overlay_pivot = ImVec2(1.0, 1.0);
        ImGui::SetNextWindowPos(overlay_pos, ImGuiCond_Always, overlay_pivot);
        if (ImGui::Begin("Hints", nullptr, flags)) {
            ImGui::Text("Shortcuts");
            const ImVec4 on_color  = ImVec4(0.0, 1.0, 0.0, 1.0);
            const ImVec4 off_color = ImVec4(1.0, 0.0, 0.0, 1.0);

            if (show_demo_window)
                ImGui::TextColored(on_color, strings[0]);
            else
                ImGui::TextColored(off_color, strings[0]);

            if (show_cube_mutator)
                ImGui::TextColored(on_color, strings[1]);
            else
                ImGui::TextColored(off_color, strings[1]);

            if (camera_mouse)
                ImGui::TextColored(on_color, strings[2]);
            else
                ImGui::TextColored(off_color, strings[2]);

            ImGui::Text(strings[3]);

            ImGui::Separator();

            std::string str = fmt::format("Location: [{: 5.3F}, {: 5.3F}, {: 5.3F}]", camera.location.x,
                                          camera.location.y, camera.location.z);
            ImGui::TextUnformatted(str.c_str());

            str = fmt::format("Rotation: [{:>7.3F}, {:>7.3F}]", glm::degrees(camera.rotation.x),
                              glm::degrees(camera.rotation.y));
            ImGui::TextUnformatted(str.c_str());

            str = fmt::format("FOV:      {:.1F}", fov);
            ImGui::TextUnformatted(str.c_str());
        }
        ImGui::End();
    }

    void draw_ui()
    {
        if (camera_mouse)
            ImGui::BeginDisabled();

        if (show_cube_mutator)
            cube_mutator();

        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        draw_hint_box();

        if (camera_mouse)
            ImGui::EndDisabled();
    }

    void process(double delta) override
    {
        draw_ui();

        glm::vec3 transform = {get_axis(KeyboardKey::D, KeyboardKey::A, KeyboardKey::W, KeyboardKey::S),
                               get_magnitude(KeyboardKey::Q, KeyboardKey::E)};

        if (glm::dot(transform, transform) > FLT_EPSILON) {
            float magnitude = MOTION_SPEED * delta;
            if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT))
                magnitude *= 2.0;

            camera.location +=
                glm::normalize(glm::vec3(camera.get_facing_matrix() * glm::vec4(transform, 1.0))) * magnitude;
        }
        get_render_backend().update_view(camera);

        for (auto &object : objects)
            object->physics_process(delta);
    }

    void handle_draw(struct engine::DrawingContext &ctx) override
    {
        for (auto &obj : objects)
            obj->draw(ctx);
    }

    CameraTransform          camera;
    shared_ptr<Cube>         cube    = nullptr;
    shared_ptr<Cube>         cube_2  = nullptr;
    list<shared_ptr<Object>> objects = {};

    bool  camera_mouse = true;
    float fov          = DEFAULT_FOV;
};

static void set_spdlog_global_settings()
{
    spdlog::set_pattern("[%D %T - %n: %^%l%$] %v");
}

static shared_ptr<spdlog::logger> make_logger()
{
    using stdout_color_sink = spdlog::sinks::stdout_color_sink_mt;
    auto stdout_sink        = std::make_shared<stdout_color_sink>();

    if constexpr (DEBUG_ASSERTIONS)
        stdout_sink->set_level(spdlog::level::debug);
    else
        stdout_sink->set_level(spdlog::level::warn);

    initializer_list<spdlog::sink_ptr> sinks = {stdout_sink};

    auto logger = std::make_shared<spdlog::logger>("application", sinks);
    spdlog::initialize_logger(logger);

    return logger;
}

MAIN
{
    set_spdlog_global_settings();
    auto logger = make_logger();
    spdlog::set_default_logger(logger);

    try {
        auto window = ExampleWindow("Window", 800, 600);
        window.show();
        window.run();
    } catch (engine::Exception &e) {
        e.log();
    }

    logger->info("Done");
}
