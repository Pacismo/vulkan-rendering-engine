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
using engine::primitives::GouraudVertex, engine::Object;
using std::shared_ptr, std::initializer_list, engine::Window, std::string_view, std::array, std::chrono::time_point,
    std::chrono::system_clock, std::list, std::shared_ptr;

const array<GouraudVertex, 4> VERTICES = {
    GouraudVertex {.position = {-0.5, -0.5, 0.0}, .color = {1.0, 0.0, 0.0}},
    GouraudVertex { .position = {0.5, -0.5, 0.0}, .color = {0.0, 1.0, 0.0}},
    GouraudVertex {  .position = {0.5, 0.5, 0.0}, .color = {0.0, 0.0, 1.0}},
    GouraudVertex { .position = {-0.5, 0.5, 0.0}, .color = {1.0, 1.0, 1.0}},
};

const array<uint32_t, 6> INDICES = {0, 1, 2, 2, 3, 0};

class ExampleWindow : public Window
{
  public:
    ExampleWindow(string_view title, int width, int height)
        : Window(title, width, height, "Runtime", {0, 1, 0})
    {
        auto rb = get_render_backend();

        auto vertices = VERTICES;
        auto indices  = INDICES;

        objects.push_back(rb->load(vertices, indices));
    }

    void physics_process(double delta) override { }

    void handle_draw(struct engine::DrawingContext &ctx) override
    {
        for (auto &obj : objects)
            obj->draw(ctx);
    }

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