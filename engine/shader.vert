#version 450

layout(binding = 0) uniform ViewProjectionUniform {
	mat4 view;
	mat4 projection;
} vpu;

layout(binding = 1) uniform ModelProjection {
	mat4 mat;
} model;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;

layout(location = 0) out vec3 out_color;

void main() {
	gl_Position = vpu.projection * vpu.view * model.mat * vec4(in_position, 1.0);
	out_color = in_color;
}
