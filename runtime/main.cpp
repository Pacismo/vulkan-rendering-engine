#include <algorithm>
#include <exceptions.hpp>
#include <fmt/format.h>
#include <memory>
#include <span>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <vertex.hpp>
#include <window.hpp>

using std::shared_ptr, std::initializer_list, engine::Window, std::string_view, std::array, engine::primitives::Vertex;
using MeshHandle = engine::RenderBackend::MeshHandlePtr;

// TODO: load into a framebuffer and render it
const array<Vertex, 4> VERTICES = {
    Vertex {.position = {-0.5, -0.5, 0.0}, .color = {1.0, 0.0, 0.0}},
    Vertex { .position = {0.5, -0.5, 0.0}, .color = {0.0, 1.0, 0.0}},
    Vertex {  .position = {0.5, 0.5, 0.0}, .color = {0.0, 0.0, 1.0}},
    Vertex { .position = {-0.5, 0.5, 0.0}, .color = {1.0, 1.0, 1.0}},
};

const array<uint32_t, 6> INDICES = {0, 1, 2, 2, 3, 0};

class ExampleWindow : public Window
{
    MeshHandle triangle;

  public:
    ExampleWindow(string_view title, int width, int height)
        : Window(title, width, height, "Runtime", {0, 1, 0})
    {
        auto rb = get_render_backend();

        auto vertices = VERTICES;
        auto indices  = INDICES;

        triangle = rb->load(vertices, indices);
    }

    void process() override { }
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

int main()
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