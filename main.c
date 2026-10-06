#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <jml/jml.h>
#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct Vertex {
    JML_Vec3f position;
    JML_Vec3f normal;
} Vertex;

typedef struct VertexUniforms {
    JML_Mat4x4f world_camera;
    JML_Mat4x4f projection;
} VertexUniforms;

typedef struct FragmentUniforms {
    JML_Vec3f diffuse_color;
    float _pad_0;
    JML_Vec3f ambient_color;
    float _pad_1;
    JML_Vec3f specular_color;
    float specular_exponent;
    JML_Vec3f light_direction;
    float _pad_2;
} FragmentUniforms;

SDL_GPUShader* createShader(SDL_GPUDevice* device, char* path, SDL_GPUShaderStage shader_stage, Uint32 num_samplers, Uint32 num_storage_textures, Uint32 num_storage_buffers, Uint32 num_uniform_buffers) {
    size_t code_size;
    void* code = SDL_LoadFile(path, &code_size);
    if (code == nullptr) {
        SDL_Log("file='%s', line=%i, SDL_LoadFile failed: %s", __FILE__, __LINE__, SDL_GetError());
        SDL_ClearError();
        SDL_SetError("Couldn't load shader code.");
        goto load_file_failed;
    }

    const SDL_GPUShaderCreateInfo shader_info = {
        .code_size = code_size,
        .code = code,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = shader_stage,
        .num_samplers = num_samplers,
        .num_storage_textures = num_storage_textures,
        .num_storage_buffers = num_storage_buffers,
        .num_uniform_buffers = num_uniform_buffers
    };

    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shader_info);
    if (shader == nullptr) {
        SDL_Log("file='%s', line=%i, SDL_CreateGPUShader failed: %s", __FILE__, __LINE__, SDL_GetError());
        SDL_ClearError();
        SDL_SetError("Couldn't create shader.");
        goto create_shader_failed;
    }

    SDL_free(code);
    return shader;

create_shader_failed:
    SDL_free(code);
load_file_failed:
    return nullptr;
}

SDL_GPUGraphicsPipeline* createGraphicsPipeline(SDL_GPUDevice* device, SDL_GPUTextureFormat swapchain_texture_format) {
    SDL_GPUShader* vertex_shader = createShader(device, "phong.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0u, 0u, 0u, 1u);
    if (vertex_shader == nullptr) {
        SDL_Log("file='%s', line=%i, createShader failed: %s", __FILE__, __LINE__, SDL_GetError());
        SDL_ClearError();
        SDL_SetError("Couldn't create vertex shader.");
        goto create_vertex_shader_failed;
    }

    SDL_GPUShader* fragment_shader = createShader(device, "phong.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0u, 0u, 0u, 1u);
    if (fragment_shader == nullptr) {
        SDL_Log("file='%s', line=%i, createShader failed: %s", __FILE__, __LINE__, SDL_GetError());
        SDL_ClearError();
        SDL_SetError("Couldn't create fragment shader.");
        goto create_fragment_shader_failed;
    }

    const SDL_GPUGraphicsPipelineCreateInfo graphics_pipeline_create_info = {
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .vertex_input_state = {
            .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[1]){
                {
                    .slot = 0,
                    .pitch = sizeof(Vertex),
                    .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                    .instance_step_rate = 0,
                }
            },
            .num_vertex_buffers = 1,
            .vertex_attributes = (SDL_GPUVertexAttribute[2]){
                {
                    .location = 0,
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                    .offset = 0,
                },
                {
                    .location = 1,
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                    .offset = sizeof(JML_Vec3f),
                },
            },
            .num_vertex_attributes = 2,
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = {
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .cull_mode = SDL_GPU_CULLMODE_BACK,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
        },
        .target_info = {
            .color_target_descriptions = (SDL_GPUColorTargetDescription[1]){
                {
                    .format = swapchain_texture_format,
                    .blend_state = {
                        .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                        .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                        .color_blend_op = SDL_GPU_BLENDOP_ADD,
                        .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                        .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                        .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                        .enable_blend = true,
                    },
                },
            },
            .num_color_targets = 1,
        },
    };

    SDL_GPUGraphicsPipeline* graphics_pipeline = SDL_CreateGPUGraphicsPipeline(device, &graphics_pipeline_create_info);
    if (graphics_pipeline == nullptr) {
        SDL_Log("file='%s', line=%i, SDL_CreateGPUGraphicsPipeline failed: %s", __FILE__, __LINE__, SDL_GetError());
        SDL_ClearError();
        SDL_SetError("Couldn't create graphics pipeline.");
        goto create_gpu_graphics_pipeline_failed;
    }

    SDL_ReleaseGPUShader(device, fragment_shader);
    SDL_ReleaseGPUShader(device, vertex_shader);
    return graphics_pipeline;

create_gpu_graphics_pipeline_failed:
    SDL_ReleaseGPUShader(device, fragment_shader);
create_fragment_shader_failed:
    SDL_ReleaseGPUShader(device, vertex_shader);
create_vertex_shader_failed:
    return nullptr;
}

SDL_GPUBuffer* createVertexBuffer(SDL_GPUDevice* device) {
    const Vertex VERTICES[36] = {
        // +x
        {JML_vec3f(1.f, 1.f, 1.f), JML_vec3f(1.f, 0.f, 0.f)},
        {JML_vec3f(1.f, -1.f, 1.f), JML_vec3f(1.f, 0.f, 0.f)},
        {JML_vec3f(1.f, 1.f, -1.f), JML_vec3f(1.f, 0.f, 0.f)},
        {JML_vec3f(1.f, -1.f, -1.f), JML_vec3f(1.f, 0.f, 0.f)},
        {JML_vec3f(1.f, 1.f, -1.f), JML_vec3f(1.f, 0.f, 0.f)},
        {JML_vec3f(1.f, -1.f, 1.f), JML_vec3f(1.f, 0.f, 0.f)},
        // -x
        {JML_vec3f(-1.f, 1.f, 1.f), JML_vec3f(-1.f, 0.f, 0.f)},
        {JML_vec3f(-1.f, 1.f, -1.f), JML_vec3f(-1.f, 0.f, 0.f)},
        {JML_vec3f(-1.f, -1.f, 1.f), JML_vec3f(-1.f, 0.f, 0.f)},
        {JML_vec3f(-1.f, -1.f, -1.f), JML_vec3f(-1.f, 0.f, 0.f)},
        {JML_vec3f(-1.f, -1.f, 1.f), JML_vec3f(-1.f, 0.f, 0.f)},
        {JML_vec3f(-1.f, 1.f, -1.f), JML_vec3f(-1.f, 0.f, 0.f)},
        // +y
        {JML_vec3f(1.f, 1.f, 1.f), JML_vec3f(0.f, 1.f, 0.f)},
        {JML_vec3f(1.f, 1.f, -1.f), JML_vec3f(0.f, 1.f, 0.f)},
        {JML_vec3f(-1.f, 1.f, 1.f), JML_vec3f(0.f, 1.f, 0.f)},
        {JML_vec3f(-1.f, 1.f, -1.f), JML_vec3f(0.f, 1.f, 0.f)},
        {JML_vec3f(-1.f, 1.f, 1.f), JML_vec3f(0.f, 1.f, 0.f)},
        {JML_vec3f(1.f, 1.f, -1.f), JML_vec3f(0.f, 1.f, 0.f)},
        // -y
        {JML_vec3f(1.f, -1.f, 1.f), JML_vec3f(0.f, -1.f, 0.f)},
        {JML_vec3f(-1.f, -1.f, 1.f), JML_vec3f(0.f, -1.f, 0.f)},
        {JML_vec3f(1.f, -1.f, -1.f), JML_vec3f(0.f, -1.f, 0.f)},
        {JML_vec3f(-1.f, -1.f, -1.f), JML_vec3f(0.f, -1.f, 0.f)},
        {JML_vec3f(1.f, -1.f, -1.f), JML_vec3f(0.f, -1.f, 0.f)},
        {JML_vec3f(-1.f, -1.f, 1.f), JML_vec3f(0.f, -1.f, 0.f)},
        // +z
        {JML_vec3f(1.f, 1.f, 1.f), JML_vec3f(0.f, 0.f, 1.f)},
        {JML_vec3f(-1.f, 1.f, 1.f), JML_vec3f(0.f, 0.f, 1.f)},
        {JML_vec3f(1.f, -1.f, 1.f), JML_vec3f(0.f, 0.f, 1.f)},
        {JML_vec3f(-1.f, -1.f, 1.f), JML_vec3f(0.f, 0.f, 1.f)},
        {JML_vec3f(1.f, -1.f, 1.f), JML_vec3f(0.f, 0.f, 1.f)},
        {JML_vec3f(-1.f, 1.f, 1.f), JML_vec3f(0.f, 0.f, 1.f)},
        // -z
        {JML_vec3f(1.f, 1.f, -1.f), JML_vec3f(0.f, 0.f, -1.f)},
        {JML_vec3f(1.f, -1.f, -1.f), JML_vec3f(0.f, 0.f, -1.f)},
        {JML_vec3f(-1.f, 1.f, -1.f), JML_vec3f(0.f, 0.f, -1.f)},
        {JML_vec3f(-1.f, -1.f, -1.f), JML_vec3f(0.f, 0.f, -1.f)},
        {JML_vec3f(-1.f, 1.f, -1.f), JML_vec3f(0.f, 0.f, -1.f)},
        {JML_vec3f(1.f, -1.f, -1.f), JML_vec3f(0.f, 0.f, -1.f)},
    };

    const SDL_GPUBufferCreateInfo buffer_create_info = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = 36 * sizeof(Vertex),
    };

    SDL_GPUBuffer* vertex_buffer = SDL_CreateGPUBuffer(device, &buffer_create_info);
    if (vertex_buffer == nullptr) {
        SDL_Log("file='%s', line=%i, SDL_CreateGPUBuffer failed: %s", __FILE__, __LINE__, SDL_GetError());
        SDL_ClearError();
        SDL_SetError("Couldn't create vertex buffer.");
        goto create_vertex_buffer_failed;
    }

    SDL_GPUTransferBufferCreateInfo transfer_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = 36 * sizeof(Vertex),
    };

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    if (transfer_buffer == nullptr) {
        SDL_Log("file='%s', line=%i, SDL_CreateGPUTransferBuffer failed: %s", __FILE__, __LINE__, SDL_GetError());
        SDL_ClearError();
        SDL_SetError("Couldn't create transfer buffer.");
        goto create_transfer_buffer_failed;
    }

    void* transfer_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    if (transfer_data == nullptr) {
        SDL_Log("file='%s', line=%i, SDL_MapGPUTransferBuffer failed: %s", __FILE__, __LINE__, SDL_GetError());
        SDL_ClearError();
        SDL_SetError("Couldn't map transfer buffer.");
        goto map_transfer_buffer_failed;
    }

    SDL_memcpy(transfer_data, VERTICES, 36 * sizeof(Vertex));
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (command_buffer == nullptr) {
        SDL_Log("file='%s', line=%i, SDL_AcquireGPUCommandBuffer failed: %s", __FILE__, __LINE__, SDL_GetError());
        SDL_ClearError();
        SDL_SetError("Couldn't acquire command buffer.");
        goto acquire_command_buffer_failed;
    }

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);

    const SDL_GPUTransferBufferLocation location = {
        .transfer_buffer = transfer_buffer,
        .offset = 0,
    };

    const SDL_GPUBufferRegion region = {
        .buffer = vertex_buffer,
        .offset = 0,
        .size = 36 * sizeof(Vertex),
    };

    SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);

    SDL_EndGPUCopyPass(copy_pass);

    if (!SDL_SubmitGPUCommandBuffer(command_buffer)) {
        SDL_Log("file='%s', line=%i, SDL_SubmitGPUCommandBuffer failed: %s", __FILE__, __LINE__, SDL_GetError());
        SDL_ClearError();
        SDL_SetError("Couldn't submit command buffer.");
        goto submit_command_buffer_failed;
    }

    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    return vertex_buffer;

submit_command_buffer_failed:
acquire_command_buffer_failed:
map_transfer_buffer_failed:
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
create_transfer_buffer_failed:
    SDL_ReleaseGPUBuffer(device, vertex_buffer);
create_vertex_buffer_failed:
    return nullptr;
}

static void dump(const char* name, const JML_Mat4x4f* m) {
    const float* f = (const float*)m;
    printf("%s (4 floats per line = one column in memory):\n", name);
    for (int i = 0; i < 16; i++)
        printf("%8g%c", f[i], (i % 4 == 3) ? '\n' : ' ');
}

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("file='%s', line=%i, SDL_Init failed: %s", __FILE__, __LINE__, SDL_GetError());
        SDL_ClearError();
        goto init_failed;
    }

    const int WINDOW_WIDTH = 600;
    const int WINDOW_HEIGHT = 450;

    SDL_Window* window = SDL_CreateWindow("GPU Example", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
    if (window == nullptr) {
        SDL_Log("file='%s', line=%i, SDL_CreateWindow failed: %s", __FILE__, __LINE__, SDL_GetError());
        SDL_ClearError();
        goto create_window_failed;
    }

    SDL_GPUDevice* device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr);
    if (device == nullptr) {
        SDL_Log("file='%s', line=%i, SDL_CreateGPUDevice failed: %s", __FILE__, __LINE__, SDL_GetError());
        SDL_ClearError();
        goto create_gpu_device_failed;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        SDL_Log("file='%s', line=%i, SDL_ClaimWindowForGPUDevice failed: %s", __FILE__, __LINE__, SDL_GetError());
        SDL_ClearError();
        goto claim_window_failed;
    }

    SDL_GPUTextureFormat swapchain_texture_format = SDL_GetGPUSwapchainTextureFormat(device, window);

    SDL_GPUGraphicsPipeline* graphics_pipeline = createGraphicsPipeline(device, swapchain_texture_format);
    if (graphics_pipeline == nullptr) {
        SDL_Log("file='%s', line=%i, createGraphicsPipeline failed: %s", __FILE__, __LINE__, SDL_GetError());
        SDL_ClearError();
        goto create_graphics_pipeline_failed;
    }

    SDL_GPUBuffer* vertex_buffer = createVertexBuffer(device);
    if (vertex_buffer == nullptr) {
        SDL_Log("file='%s', line=%i, createVertexBuffer failed: %s", __FILE__, __LINE__, SDL_GetError());
        SDL_ClearError();
        goto create_vertex_buffer_failed;
    }

    SDL_ShowWindow(window);

    bool running = true;
    JML_Vec2f angles = JML_vec2f(0.f, 0.f);

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT: {
                    running = false;
                    break;
                }
                case SDL_EVENT_MOUSE_MOTION: {
                    if ((event.motion.state & SDL_BUTTON_LMASK) != 0) {
                        int window_size_y;

                        SDL_GetWindowSize(window, nullptr, &window_size_y);
                        JML_Vec2f delta = JML_div(JML_vec2f(event.motion.xrel, event.motion.yrel), (float)(window_size_y));

                        angles.x = (float)fmod(angles.x + delta.x + 2.f * M_PI, 2.f * M_PI);
                        angles.y = (float)fmin(fmax(angles.y + delta.y, -0.5f * M_PI), 0.5f * M_PI);
                    }
                    break;
                }
                default: {
                    break;
                }
            }
        }

        SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
        if (command_buffer == nullptr) {
            SDL_Log("file='%s', line=%i, SDL_AcquireGPUCommandBuffer failed: %s", __FILE__, __LINE__, SDL_GetError());
            SDL_ClearError();
            continue;
        }

        SDL_GPUTexture* swapchain_texture = nullptr;
        Uint32 size_x = 0;
        Uint32 size_y = 0;

        if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, &size_x, &size_y)) {
            SDL_Log("file='%s', line=%i, SDL_WaitAndAcquireGPUSwapchainTexture failed: %s", __FILE__, __LINE__, SDL_GetError());
            SDL_ClearError();
            SDL_CancelGPUCommandBuffer(command_buffer);
            continue;
        }

        if (swapchain_texture != nullptr && size_x > 0 && size_y > 0) {
            const SDL_FColor background_color = {
                0.2f, 0.51f, 0.88f, 1.f
            };

            SDL_GPUColorTargetInfo targets[1] = {
                {
                    .texture = swapchain_texture,
                    .clear_color = background_color,
                    .load_op = SDL_GPU_LOADOP_CLEAR,
                    .store_op = SDL_GPU_STOREOP_STORE,
                },
            };
            SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(command_buffer, targets, 1, nullptr);

            SDL_BindGPUGraphicsPipeline(pass, graphics_pipeline);

            float aspect_ratio = (float)size_x / (float)size_y;

            float camera_distance = 5.f;

            JML_Mat4x4f world_camera = JML_translation(JML_vec3f(0.f, 0.f, -camera_distance));
            world_camera = JML_mul(world_camera, JML_rotation(JML_vec3f(1.f, 0.f, 0.f), angles.y));
            world_camera = JML_mul(world_camera, JML_rotation(JML_vec3f(0.f, 1.f, 0.f), angles.x));

            JML_Mat4x4f projection = JML_frustum(-aspect_ratio, aspect_ratio, -1.f, 1.f, 2.f, 7.f);

            VertexUniforms vertex_uniforms = {
                .world_camera = world_camera,
                .projection = projection,
            };

            SDL_PushGPUVertexUniformData(command_buffer, 0, &vertex_uniforms, sizeof(VertexUniforms));

            const JML_Vec4f light_direction_affine = JML_mul(world_camera, JML_vec4f(1.f, 3.f, 2.f, 0.f));
            FragmentUniforms fragment_uniforms = {
                .diffuse_color = JML_mul(1.5f, JML_vec3f(0.99f, 0.51f, 0.07f)),
                .ambient_color = JML_mul(0.5f, JML_vec3f(0.99f, 0.51f, 0.07f)),
                .specular_color = JML_mul(3.0f, JML_vec3f(1.f, 1.f, 1.f)),
                .specular_exponent = 30.f,
                .light_direction = JML_vec3f(light_direction_affine.x, light_direction_affine.y, light_direction_affine.z),
            };

            SDL_PushGPUFragmentUniformData(command_buffer, 0, &fragment_uniforms, sizeof(FragmentUniforms));

            SDL_GPUBufferBinding buffer_bindings[1] = {
                {
                    .buffer = vertex_buffer,
                    .offset = 0,
                },
            };

            SDL_BindGPUVertexBuffers(pass, 0, buffer_bindings, 1);

            SDL_DrawGPUPrimitives(pass, 36, 1, 0, 0);

            SDL_EndGPURenderPass(pass);
        }

        if (!SDL_SubmitGPUCommandBuffer(command_buffer)) {
            SDL_Log("file='%s', line=%i, SDL_SubmitGPUCommandBuffer failed: %s", __FILE__, __LINE__, SDL_GetError());
            SDL_ClearError();
        }
    }

    SDL_HideWindow(window);

    SDL_WaitForGPUIdle(device);
    SDL_ReleaseGPUBuffer(device, vertex_buffer);
    SDL_ReleaseGPUGraphicsPipeline(device, graphics_pipeline);
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;

create_vertex_buffer_failed:
    SDL_ReleaseGPUGraphicsPipeline(device, graphics_pipeline);
create_graphics_pipeline_failed:
    SDL_ReleaseWindowFromGPUDevice(device, window);
claim_window_failed:
    SDL_DestroyGPUDevice(device);
create_gpu_device_failed:
    SDL_DestroyWindow(window);
create_window_failed:
    SDL_Quit();
init_failed:
    return 1;
}