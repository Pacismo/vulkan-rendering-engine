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
using engine::primitives::GouraudVertex, engine::Object, engine::Transform;
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
    double motion = 0.0;

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
        transform.rotation.y = glm::mod<float>(transform.rotation.y + glm::radians(90.0) * delta, glm::radians(360.0));

        motion               = glm::mod<float>(motion + 1.0 * delta, 2.0);
        transform.location.x = glm::abs(motion - 1.0) * 2.0 - 1.0;
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

        rb->set_view(glm::lookAt<float, glm::highp>({2.0f, 2.0f, 2.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}));
    }

    void process(double delta) override
    {
        for (auto &object : objects)
            object->physics_process(delta);
    }

    void handle_draw(struct engine::DrawingContext &ctx) override
    {
        for (auto &obj : objects)
            obj->draw(ctx);
    }

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