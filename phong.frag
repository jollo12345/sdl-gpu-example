#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec4 fragment_color;

layout(std140, set = 3, binding = 0) uniform UniformBlock {
    vec3 diffuse_color;
    vec3 ambient_color;
    vec3 specular_color;
    float specular_exponent;
    vec3 light_direction;
};

const float EPSILON = 1e-6f;

vec3 toneMap(vec3 color) {
    const vec3 percived_brightness = vec3(0.2126f, 0.7152f, 0.0722f);
    const float luminance_in  = dot(color, percived_brightness);
    const float luminance_out = luminance_in / (1.f + luminance_in);

    const vec3 mapped = color * (luminance_out / (luminance_in + EPSILON));
    return mix(mapped, vec3(1.f), pow(luminance_out, 4.f));
}

void main() {
    const vec3 normal_norm = normalize(normal);
    const vec3 light_direction_norm = normalize(light_direction);
    const vec3 view_direction_norm = normalize(-position);

    const float diffuse = max(dot(normal_norm, light_direction_norm), 0.f);

    const vec3 reflection = normalize(reflect(-light_direction_norm, normal_norm));
    const float specular = (
        dot(normal_norm, light_direction_norm) > 0.f ?
        pow(max(dot(reflection, view_direction_norm), 0.f), specular_exponent) :
        0.f
    );

    const vec3 color = ambient_color + diffuse * diffuse_color + specular_color * specular;

    fragment_color = vec4(toneMap(color), 1.f);
}
