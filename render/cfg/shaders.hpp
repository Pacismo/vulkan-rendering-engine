#pragma once
#include <span>

constexpr uint32_t vertex_shader_data[] = {
${VERT_SHADER}
};

constexpr size_t vertex_shader_len = sizeof(vertex_shader_data) / sizeof(uint32_t);

constexpr std::span<const uint32_t, vertex_shader_len> vertex_shader(vertex_shader_data);

constexpr uint32_t fragment_shader_data[] = {
${FRAG_SHADER}
};

constexpr size_t fragment_shader_len = sizeof(fragment_shader_data) / sizeof(uint32_t);

constexpr std::span<const uint32_t, fragment_shader_len> fragment_shader(fragment_shader_data);