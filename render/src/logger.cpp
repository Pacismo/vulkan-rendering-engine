#include "logger.hpp"

std::shared_ptr<spdlog::logger> get_logger()
{
    if (auto logger = spdlog::get("render"))
        return logger;

    const auto                &sinks  = spdlog::default_logger()->sinks();
    std::shared_ptr<spdlog::logger> logger = make_shared<spdlog::logger>("render", sinks.begin(), sinks.end());
    spdlog::initialize_logger(logger);
    return logger;
}