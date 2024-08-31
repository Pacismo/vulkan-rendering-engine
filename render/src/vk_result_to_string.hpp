#pragma once
#include <string_view>
#include <vulkan/vulkan.hpp>

std::string_view result_to_string(vk::Result result);

std::string_view result_to_string(uint32_t result_code);