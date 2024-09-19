#include <algorithm>
#include <backend/vulkan_backend.hpp>
#include <chrono>
#include <exceptions.hpp>
#include <fmt/format.h>
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
using engine::primitives::GouraudVertex, engine::Object, engine::Transform, engine::CameraTransform;
using std::shared_ptr, std::initializer_list, engine::Window, std::string_view, std::array, std::chrono::time_point,
    std::chrono::system_clock, std::list, std::shared_ptr;

const array<GouraudVertex, 8> VERTICES = {
    GouraudVertex {.position = {-0.5, -0.5, 0.0}, .color = {1.0, 0.0, 0.0}},
    GouraudVertex { .position = {0.5, -0.5, 0.0}, .color = {0.0, 1.0, 0.0}},
    GouraudVertex {  .position = {0.5, 0.5, 0.0}, .color = {0.0, 0.0, 1.0}},
    GouraudVertex { .position = {-0.5, 0.5, 0.0}, .color = {1.0, 1.0, 1.0}},
    GouraudVertex {.position = {-0.5, -0.5, 1.0}, .color = {0.5, 0.0, 0.0}},
    GouraudVertex { .position = {0.5, -0.5, 1.0}, .color = {0.0, 0.5, 0.0}},
    GouraudVertex {  .position = {0.5, 0.5, 1.0}, .color = {0.0, 0.0, 0.5}},
    GouraudVertex { .position = {-0.5, 0.5, 1.0}, .color = {0.5, 0.5, 0.5}},
};

const array<uint32_t, 36> INDICES = {
    0, 1, 2, 2, 3, 0, //
    6, 5, 4, 4, 7, 6, //
    5, 1, 0, 4, 5, 0, //
    6, 2, 1, 5, 6, 1, //
    7, 3, 2, 6, 7, 2, //
    4, 0, 3, 7, 4, 3, //
};

class Cube : public Object
{
  public:
    Cube(std::shared_ptr<engine::VulkanBackend> &backend)
    {
        auto vertices = VERTICES;
        auto indices  = INDICES;

        mesh                     = backend->load(vertices, indices);
        mesh->transform.location = {0.0, 0.0, -0.5};
    }

    void physics_process(double delta) override
    {
        transform.rotation.z = glm::mod<float>(transform.rotation.z + glm::radians(180.0) * delta, glm::radians(360.0));
    }

    void draw(engine::DrawingContext &context, const glm::mat4 &) override { mesh->draw(context, transform); }

    std::shared_ptr<Object> mesh;
};

class ExampleWindow : public Window
{
  public:
    ExampleWindow(string_view title, int width, int height)
        : Window(title, width, height, "Runtime", {0, 1, 0})
    {
        auto rb = get_render_backend();

        cube = shared_ptr<Cube>(new Cube(rb));
        objects.push_back(cube);

        camera.location = {2.0, 2.0, 2.0};
        camera.rotation = {glm::radians(135.0), glm::radians(-35.0)};

        if (glfwRawMouseMotionSupported()) {
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        }

        // Force a poll to update values
        glfwPollEvents();
        glfwGetCursorPos(m_window, &last_x, &last_y);

        glfwSetCursorPosCallback(m_window, [](GLFWwindow *p_wnd, double x, double y) {
            constexpr glm::vec2 COEFFICIENT = {-0.01, -0.01};

            ExampleWindow *window = (ExampleWindow *)glfwGetWindowUserPointer(p_wnd);

            double dx      = x - window->last_x;
            double dy      = y - window->last_y;
            window->last_x = x;
            window->last_y = y;

            window->camera.rotation.x = glm::mod(window->camera.rotation.x + dx * COEFFICIENT.x, glm::radians(360.0));
            window->camera.rotation.y =
                glm::clamp(window->camera.rotation.y + dy * COEFFICIENT.y, glm::radians(-89.9), glm::radians(89.9));
        });

        rb->set_view(camera);
    }

    static constexpr float MOTION_SPEED = 1.0;

    void process(double delta) override
    {
        if (glfwGetKey(m_window, GLFW_KEY_ESCAPE))
            glfwSetWindowShouldClose(m_window, GLFW_TRUE);

        glm::vec3 transform = {0.0, 0.0, 0.0};
        if (glfwGetKey(m_window, GLFW_KEY_W))
            transform.y = +1.0;
        if (glfwGetKey(m_window, GLFW_KEY_S))
            transform.y = -1.0;
        if (glfwGetKey(m_window, GLFW_KEY_A))
            transform.x = -1.0;
        if (glfwGetKey(m_window, GLFW_KEY_D))
            transform.x = +1.0;
        if (glfwGetKey(m_window, GLFW_KEY_Q))
            transform.z = +1.0;
        if (glfwGetKey(m_window, GLFW_KEY_E))
            transform.z = -1.0;

        if (glm::dot(transform, transform) > FLT_EPSILON) {
            float magnitude = MOTION_SPEED * delta;
            if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT))
                magnitude *= 2.0;

            camera.location +=
                glm::normalize(glm::vec3(camera.get_facing_matrix() * glm::vec4(transform, 1.0))) * magnitude;
        }
        get_render_backend()->set_view(camera);

        for (auto &object : objects)
            object->physics_process(delta);

        std::string new_title =
            fmt::format("[{:3.3F}, {:3.3F}] [{:3.3F}, {:3.3F}, {:3.3F}]", glm::degrees(camera.rotation.x),
                        glm::degrees(camera.rotation.y), camera.location.x, camera.location.y, camera.location.z);
        glfwSetWindowTitle(m_window, new_title.c_str());
    }

    void handle_draw(struct engine::DrawingContext &ctx) override
    {
        for (auto &obj : objects)
            obj->draw(ctx);
    }

    CameraTransform camera;
    double          last_x = 0.0;
    double          last_y = 0.0;

    shared_ptr<Cube> cube = nullptr;

    list<shared_ptr<Object>> objects = {};
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