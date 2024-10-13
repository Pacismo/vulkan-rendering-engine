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

#include "Cube.hpp"
#include "HintBox.hpp"
#include "ObjectMutator.hpp"

#if TERMINAL_ENABLED or not defined(WIN32)
#    define MAIN int main()
#else
#    define MAIN int WinMain()
#endif

using namespace std::chrono_literals;
using engine::primitives::GouraudVertex, engine::Object, engine::CameraTransform, engine::KeyboardKey,
    engine::KeyAction, engine::ModifierKey, engine::contains, engine::DEFAULT_FOV;
using std::shared_ptr, std::initializer_list, engine::Window, std::string_view, std::array, std::vector,
    std::shared_ptr;

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
        , hint_box(camera, fov, show_demo_window, cube_mutator, camera_mouse)
        , cube_mutator(objects)
    {
        auto &rb = get_render_backend();

        cube         = shared_ptr<Cube>(new Cube(rb));
        cube->name   = "Cube 1";
        cube_2       = shared_ptr<Cube>(new Cube(rb));
        cube_2->name = "Cube 2";
        objects.push_back(cube);
        objects.push_back(cube_2);

        camera.location = {2.0, 2.0, 2.0};
        camera.rotation = {135.0_deg, -35.0_deg};

        capture_mouse(camera_mouse);

        rb.update_view(camera);
    }

    void on_key_action(KeyboardKey key, ModifierKey mods, KeyAction action, int scancode) override
    {
        using KeyboardKey::Tab, KeyboardKey::Escape, KeyboardKey::F1, KeyboardKey::F2, KeyboardKey::R, KeyboardKey::F3;

        switch (key) {
        case Escape:
            if (!!(mods & ModifierKey::Shift))
                close();
            break;
        case Tab:
            if (action == KeyAction::Press)
                capture_mouse(camera_mouse = !camera_mouse);
            break;
#ifndef NDEBUG
        case F1:
            ImGui::DebugStartItemPicker();
            break;
#endif
        case F2:
            if (action == KeyAction::Press)
                cube_mutator.visible(!cube_mutator.visible());
            break;
        case F3:
            if (action == KeyAction::Press)
                show_demo_window = !show_demo_window;
            break;
        case R:
            if (action == KeyAction::Press) {
                camera.location = {2.0, 2.0, 2.0};
                camera.rotation = {135.0_deg, -35.0_deg};
                update_fov(fov = DEFAULT_FOV);

                for (auto &object : objects) {
                    object->transform.location = {0.0, 0.0, 0.0};
                    object->transform.rotation = {0.0, 0.0, 0.0};
                    object->transform.scale    = {1.0, 1.0, 1.0};
                }
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
        update_fov(fov);
    }

    static constexpr float MOTION_SPEED = 2.5;

    bool show_demo_window = false;

    void draw_ui()
    {
        if (camera_mouse)
            ImGui::BeginDisabled();

        if (cube_mutator)
            cube_mutator.draw();

        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        hint_box.draw();

        if (camera_mouse)
            ImGui::EndDisabled();
    }

    void process(double delta) override
    {
        draw_ui();

        glm::vec3 transform = {get_axis(KeyboardKey::D, KeyboardKey::A, KeyboardKey::W, KeyboardKey::S),
                               get_magnitude(KeyboardKey::Q, KeyboardKey::E)};

        if (glm::dot(transform, transform) > FLT_EPSILON && camera_mouse) {
            float magnitude = MOTION_SPEED * delta;
            if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT))
                magnitude *= 2.0;

            camera.location +=
                glm::normalize(glm::vec3(camera.get_facing_matrix() * glm::vec4(transform, 1.0))) * magnitude;
        }
        update_view(camera);

        for (auto &object : objects)
            object->physics_process(delta);
    }

    void handle_draw(struct engine::DrawingContext &ctx) override
    {
        for (auto &obj : objects)
            obj->draw(ctx);
    }

    CameraTransform            camera;
    shared_ptr<Cube>           cube    = nullptr;
    shared_ptr<Cube>           cube_2  = nullptr;
    vector<shared_ptr<Object>> objects = {};

    bool  camera_mouse = true;
    float fov          = DEFAULT_FOV;

    HintBox       hint_box;
    ObjectMutator cube_mutator;
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
