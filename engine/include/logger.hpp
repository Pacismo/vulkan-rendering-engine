#pragma once
#include <memory>
#include <spdlog/spdlog.h>

std::shared_ptr<spdlog::logger> get_logger();