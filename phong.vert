#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec3 vertex_position;
layout(location = 1) out vec3 vertex_normal;

layout(std140, set = 1, binding = 0) uniform UniformBlock {
    mat4 world_camera;
    mat4 projection;
};

void main() {
    gl_Position = projection * world_camera * vec4(position, 1.f);

    vec4 vertex_position_affine = world_camera * vec4(position, 1.f);
    vertex_position = vertex_position_affine.xyz / vertex_position_affine.w;

    vec4 vertex_normal_affine = world_camera * vec4(normal, 0.f);
    vertex_normal = vertex_normal_affine.xyz;
}
