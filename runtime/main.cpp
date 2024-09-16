#include <algorithm>
#include <chrono>
#include <exceptions.hpp>
#include <fmt/format.h>
#include <memory>
#include <object.hpp>
#include <span>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <vertex.hpp>
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
};

const array<uint32_t, 12> INDICES = {
    0, 1, 2, 2, 3, 0,
};

class Cube : public Object
{
  public:
    Cube(std::shared_ptr<engine::RenderBackend> &backend)
    {
        auto vertices = VERTICES;
        auto indices  = INDICES;

        squares[0]                       = backend->load(vertices, indices);
        squares[0]->transform.location   = {0.5, 0.0, 0.5};
        squares[0]->transform.rotation.x = glm::radians(90.0);
        squares[1]                       = backend->load(vertices, indices);
        squares[1]->transform.location   = {-0.5, 0.0, 0.5};
        squares[1]->transform.rotation.x = glm::radians(-90.0);

        squares[2]                       = backend->load(vertices, indices);
        squares[2]->transform.location   = {0.0, -0.5, 0.5};
        squares[2]->transform.rotation.y = glm::radians(90.0);
        squares[3]                       = backend->load(vertices, indices);
        squares[3]->transform.location   = {0.0, 0.5, 0.5};
        squares[3]->transform.rotation.y = glm::radians(-90.0);

        squares[4]                       = backend->load(vertices, indices);
        squares[4]->transform.location   = {0.0, 0.0, 0.0};
        squares[4]->transform.rotation.x = glm::radians(180.0);
        squares[5]                       = backend->load(vertices, indices);
        squares[5]->transform.location   = {0.0, 0.0, 1.0};

        transform.location.z = -0.5;
    }

    void physics_process(double delta) override
    {
        float sqr            = transform.rotation.z + glm::radians(180.0) * delta;
        transform.rotation.z = glm::mod<float>(sqr, glm::radians(360.0));
    }

    void draw(engine::DrawingContext &context, const glm::mat4 &) override
    {
        for (auto &square : squares)
            square->draw(context, transform);
    }

    std::shared_ptr<Object> squares[6];
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