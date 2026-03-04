#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

// Debuging Macro
namespace {
template<typename Type>
constexpr Type check_error(Type result, const char* expression_name, const char* file_name, int line) {
    if(!result) {
        std::cerr << "'" << expression_name << "' failed in file \"" << file_name << "\" at line " << line << ": " << SDL_GetError() << std::endl;
    }
    return result;
};
} // namespace

#ifndef NDEBUG
#define CHECK_ERROR(expression) ::check_error((expression), #expression, __FILE__, __LINE__)
#else
#define CHECK_ERROR(expression) (expression)
#endif

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

struct VertexUniforms {
    glm::mat4 world_camera;
    glm::mat4 projection;
};

struct FragmentUniforms {
    glm::vec3 diffuse_color;
    float _pad_0;
    glm::vec3 ambient_color;
    float _pad_1;
    glm::vec3 specular_color;
    float specular_exponent;
    glm::vec3 light_direction;
    float _pad_2;
};

struct State {
    SDL_Window* window;
    SDL_GPUDevice* device;
    SDL_GPUGraphicsPipeline* graphics_pipeline;
    SDL_GPUBuffer* vertex_buffer;

    glm::vec2 angles;
};

constexpr std::array<Vertex, 36> VERTICES{
    // +x
    Vertex(glm::vec3(1.f, 1.f, 1.f), glm::vec3(1.f, 0.f, 0.f)),
    Vertex(glm::vec3(1.f, -1.f, 1.f), glm::vec3(1.f, 0.f, 0.f)),
    Vertex(glm::vec3(1.f, 1.f, -1.f), glm::vec3(1.f, 0.f, 0.f)),
    Vertex(glm::vec3(1.f, -1.f, -1.f), glm::vec3(1.f, 0.f, 0.f)),
    Vertex(glm::vec3(1.f, 1.f, -1.f), glm::vec3(1.f, 0.f, 0.f)),
    Vertex(glm::vec3(1.f, -1.f, 1.f), glm::vec3(1.f, 0.f, 0.f)),
    // -x
    Vertex(glm::vec3(-1.f, 1.f, 1.f), glm::vec3(-1.f, 0.f, 0.f)),
    Vertex(glm::vec3(-1.f, 1.f, -1.f), glm::vec3(-1.f, 0.f, 0.f)),
    Vertex(glm::vec3(-1.f, -1.f, 1.f), glm::vec3(-1.f, 0.f, 0.f)),
    Vertex(glm::vec3(-1.f, -1.f, -1.f), glm::vec3(-1.f, 0.f, 0.f)),
    Vertex(glm::vec3(-1.f, -1.f, 1.f), glm::vec3(-1.f, 0.f, 0.f)),
    Vertex(glm::vec3(-1.f, 1.f, -1.f), glm::vec3(-1.f, 0.f, 0.f)),
    // +y
    Vertex(glm::vec3(1.f, 1.f, 1.f), glm::vec3(0.f, 1.f, 0.f)),
    Vertex(glm::vec3(1.f, 1.f, -1.f), glm::vec3(0.f, 1.f, 0.f)),
    Vertex(glm::vec3(-1.f, 1.f, 1.f), glm::vec3(0.f, 1.f, 0.f)),
    Vertex(glm::vec3(-1.f, 1.f, -1.f), glm::vec3(0.f, 1.f, 0.f)),
    Vertex(glm::vec3(-1.f, 1.f, 1.f), glm::vec3(0.f, 1.f, 0.f)),
    Vertex(glm::vec3(1.f, 1.f, -1.f), glm::vec3(0.f, 1.f, 0.f)),
    // -y
    Vertex(glm::vec3(1.f, -1.f, 1.f), glm::vec3(0.f, -1.f, 0.f)),
    Vertex(glm::vec3(-1.f, -1.f, 1.f), glm::vec3(0.f, -1.f, 0.f)),
    Vertex(glm::vec3(1.f, -1.f, -1.f), glm::vec3(0.f, -1.f, 0.f)),
    Vertex(glm::vec3(-1.f, -1.f, -1.f), glm::vec3(0.f, -1.f, 0.f)),
    Vertex(glm::vec3(1.f, -1.f, -1.f), glm::vec3(0.f, -1.f, 0.f)),
    Vertex(glm::vec3(-1.f, -1.f, 1.f), glm::vec3(0.f, -1.f, 0.f)),
    // +z
    Vertex(glm::vec3(1.f, 1.f, 1.f), glm::vec3(0.f, 0.f, 1.f)),
    Vertex(glm::vec3(-1.f, 1.f, 1.f), glm::vec3(0.f, 0.f, 1.f)),
    Vertex(glm::vec3(1.f, -1.f, 1.f), glm::vec3(0.f, 0.f, 1.f)),
    Vertex(glm::vec3(-1.f, -1.f, 1.f), glm::vec3(0.f, 0.f, 1.f)),
    Vertex(glm::vec3(1.f, -1.f, 1.f), glm::vec3(0.f, 0.f, 1.f)),
    Vertex(glm::vec3(-1.f, 1.f, 1.f), glm::vec3(0.f, 0.f, 1.f)),
    // -z
    Vertex(glm::vec3(1.f, 1.f, -1.f), glm::vec3(0.f, 0.f, -1.f)),
    Vertex(glm::vec3(1.f, -1.f, -1.f), glm::vec3(0.f, 0.f, -1.f)),
    Vertex(glm::vec3(-1.f, 1.f, -1.f), glm::vec3(0.f, 0.f, -1.f)),
    Vertex(glm::vec3(-1.f, -1.f, -1.f), glm::vec3(0.f, 0.f, -1.f)),
    Vertex(glm::vec3(-1.f, 1.f, -1.f), glm::vec3(0.f, 0.f, -1.f)),
    Vertex(glm::vec3(1.f, -1.f, -1.f), glm::vec3(0.f, 0.f, -1.f)),
};

constexpr float CAMERA_DISTANCE = 4.5f;

constexpr glm::ivec2 WINDOW_SIZE = glm::ivec2(600, 450);

constexpr glm::vec3 BACKGROUND_COLOR = glm::vec3(0.2f, 0.51f, 0.88f);

constexpr glm::vec3 DIFFUSE_COLOR = 1.5f * glm::vec3(0.99f, 0.51f, 0.07f);
constexpr glm::vec3 AMBIENT_COLOR = 0.5f * glm::vec3(0.99f, 0.51f, 0.07f);
constexpr glm::vec3 SPECULAR_COLOR = 3.0f * glm::vec3(1.f, 1.f, 1.f);
constexpr float SPECULAR_EXPONENT = 30.f;
constexpr glm::vec3 LIGHT_DIRECTION = glm::vec3(1.f, 3.f, 2.f);

SDL_AppResult SDL_AppInit(void** app_state, int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);

    State* state = new State();

    *app_state = static_cast<void*>(state);

    state->window = CHECK_ERROR(SDL_CreateWindow("GPU Example", WINDOW_SIZE.x, WINDOW_SIZE.y, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN));
    state->device = CHECK_ERROR(SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr));

    SDL_ClaimWindowForGPUDevice(state->device, state->window);

    { // Create Render Pipeline

        // Load Vertex Shader
        size_t vertex_code_size;
        void* vertex_code = CHECK_ERROR(SDL_LoadFile("phong.vert.spv", &vertex_code_size));

        SDL_GPUShaderCreateInfo vertex_info{
            .code_size = vertex_code_size,
            .code = static_cast<std::uint8_t*>(vertex_code),
            .entrypoint = "main",
            .format = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage = SDL_GPU_SHADERSTAGE_VERTEX,
            .num_samplers = 0,
            .num_storage_textures = 0,
            .num_storage_buffers = 0,
            .num_uniform_buffers = 1,
        };

        SDL_GPUShader* gpu_vertex_shader = CHECK_ERROR(SDL_CreateGPUShader(state->device, &vertex_info));

        SDL_free(vertex_code);

        // Load Fragment Shader
        size_t fragment_code_size;
        void* fragment_code = CHECK_ERROR(SDL_LoadFile("phong.frag.spv", &fragment_code_size));

        SDL_GPUShaderCreateInfo fragment_info{
            .code_size = fragment_code_size,
            .code = static_cast<std::uint8_t*>(fragment_code),
            .entrypoint = "main",
            .format = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
            .num_samplers = 0,
            .num_storage_textures = 0,
            .num_storage_buffers = 0,
            .num_uniform_buffers = 1,
        };

        SDL_GPUShader* gpu_fragment_shader = CHECK_ERROR(SDL_CreateGPUShader(state->device, &fragment_info));

        SDL_free(fragment_code);

        // Configure Graphics Pipeline
        std::vector<SDL_GPUVertexBufferDescription> vertex_buffer_desctiptions{
            SDL_GPUVertexBufferDescription{
                .slot = 0,
                .pitch = sizeof(Vertex),
                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                .instance_step_rate = 0,
            }
        };

        std::vector<SDL_GPUVertexAttribute> vertex_attributes{
            SDL_GPUVertexAttribute{
                .location = 0,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                .offset = 0,
            },
            SDL_GPUVertexAttribute{
                .location = 1,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                .offset = sizeof(glm::vec3),
            },
        };

        SDL_GPUTextureFormat swapchain_texture_format = SDL_GetGPUSwapchainTextureFormat(state->device, state->window);

        std::vector<SDL_GPUColorTargetDescription> color_target_descriptions{
            SDL_GPUColorTargetDescription{
                .format = swapchain_texture_format,
                .blend_state{
                    .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                    .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .color_blend_op = SDL_GPU_BLENDOP_ADD,
                    .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                    .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                    .enable_blend = true,
                },
            },
        };

        SDL_GPUGraphicsPipelineCreateInfo graphics_pipeline_create_info{
            .vertex_shader = gpu_vertex_shader,
            .fragment_shader = gpu_fragment_shader,
            .vertex_input_state{
                .vertex_buffer_descriptions = vertex_buffer_desctiptions.data(),
                .num_vertex_buffers = static_cast<Uint32>(vertex_buffer_desctiptions.size()),
                .vertex_attributes = vertex_attributes.data(),
                .num_vertex_attributes = static_cast<Uint32>(vertex_attributes.size()),
            },
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .rasterizer_state{
                .fill_mode = SDL_GPU_FILLMODE_FILL,
                .cull_mode = SDL_GPU_CULLMODE_BACK,
                .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
            },
            .target_info{
                .color_target_descriptions = color_target_descriptions.data(),
                .num_color_targets = static_cast<Uint32>(color_target_descriptions.size()),
            },
        };

        state->graphics_pipeline = CHECK_ERROR(SDL_CreateGPUGraphicsPipeline(state->device, &graphics_pipeline_create_info));

        SDL_ReleaseGPUShader(state->device, gpu_vertex_shader);
        SDL_ReleaseGPUShader(state->device, gpu_fragment_shader);
    }

    { // Create Vertex Buffer

        SDL_GPUBufferCreateInfo buffer_create_info{
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = static_cast<Uint32>(VERTICES.size() * sizeof(Vertex)),
        };

        state->vertex_buffer = CHECK_ERROR(SDL_CreateGPUBuffer(state->device, &buffer_create_info));

        // Copy Data From CPU To GPU
        SDL_GPUTransferBufferCreateInfo transfer_info{
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = static_cast<Uint32>(VERTICES.size() * sizeof(Vertex)),
        };

        SDL_GPUTransferBuffer* transfer_buffer = CHECK_ERROR(SDL_CreateGPUTransferBuffer(state->device, &transfer_info));

        void* transfer_data = CHECK_ERROR(SDL_MapGPUTransferBuffer(state->device, transfer_buffer, false));
        SDL_memcpy(transfer_data, VERTICES.data(), static_cast<Uint32>(VERTICES.size() * sizeof(Vertex)));
        SDL_UnmapGPUTransferBuffer(state->device, transfer_buffer);

        SDL_GPUCommandBuffer* command_buffer = CHECK_ERROR(SDL_AcquireGPUCommandBuffer(state->device));
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);

        SDL_GPUTransferBufferLocation location{
            .transfer_buffer = transfer_buffer,
            .offset = 0,
        };

        SDL_GPUBufferRegion region{
            .buffer = state->vertex_buffer,
            .offset = 0,
            .size = static_cast<Uint32>(VERTICES.size() * sizeof(Vertex)),
        };

        SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);

        SDL_EndGPUCopyPass(copy_pass);
        SDL_SubmitGPUCommandBuffer(command_buffer);

        SDL_ReleaseGPUTransferBuffer(state->device, transfer_buffer);
    }

    SDL_ShowWindow(state->window);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* app_state, SDL_Event* event) {
    State* const state = static_cast<State*>(app_state);

    switch(event->type) {
    case SDL_EVENT_QUIT: {
        return SDL_APP_SUCCESS;
    }
    case SDL_EVENT_MOUSE_MOTION: {
        if((event->motion.state & SDL_BUTTON_LMASK) != 0) {
            int window_size_y;

            CHECK_ERROR(SDL_GetWindowSize(state->window, nullptr, &window_size_y));

            auto delta = glm::vec2(event->motion.xrel, event->motion.yrel) / static_cast<float>(window_size_y) * glm::pi<float>();

            state->angles.x = glm::mod(state->angles.x + delta.x, 2.f * glm::pi<float>());
            state->angles.y = glm::clamp(state->angles.y + delta.y, -0.5f * glm::pi<float>(), 0.5f * glm::pi<float>());

            std::cout << "Camera angles set to: (" << state->angles.x << ", " << state->angles.y << ")" << std::endl;
        }
        return SDL_APP_CONTINUE;
    }
    default: {
        return SDL_APP_CONTINUE;
    }
    }
}

SDL_AppResult SDL_AppIterate(void* app_state) {
    State* const state = static_cast<State*>(app_state);

    SDL_GPUCommandBuffer* command_buffer = CHECK_ERROR(SDL_AcquireGPUCommandBuffer(state->device));

    SDL_GPUTexture* swapchain_texture;
    glm::uvec2 size;

    CHECK_ERROR(SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, state->window, &swapchain_texture, &size.x, &size.y));

    if(size.x > 0 && size.y > 0) {
        std::vector<SDL_GPUColorTargetInfo> targets{
            SDL_GPUColorTargetInfo{
                .texture = swapchain_texture,
                .clear_color = SDL_FColor(BACKGROUND_COLOR.r, BACKGROUND_COLOR.g, BACKGROUND_COLOR.b, 1.f),
                .load_op = SDL_GPU_LOADOP_CLEAR,
                .store_op = SDL_GPU_STOREOP_STORE,
            },
        };
        SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(command_buffer, targets.data(), targets.size(), nullptr);

        SDL_BindGPUGraphicsPipeline(pass, state->graphics_pipeline);

        // Setup Vertex Uniforms
        glm::ivec2 window_size;

        CHECK_ERROR(SDL_GetWindowSize(state->window, &window_size.x, &window_size.y));

        float aspect_ratio = static_cast<float>(window_size.x) / static_cast<float>(window_size.y);

        glm::mat4 world_camera = glm::identity<glm::mat4>();
        world_camera = glm::translate(world_camera, glm::vec3(0.f, 0.f, -CAMERA_DISTANCE));
        world_camera = glm::rotate(world_camera, state->angles.y, glm::vec3(1.f, 0.f, 0.f));
        world_camera = glm::rotate(world_camera, state->angles.x, glm::vec3(0.f, 1.f, 0.f));

        glm::mat4 projection = glm::frustum<float>(-aspect_ratio, aspect_ratio, -1.f, 1.f, 2.f, 7.f);

        VertexUniforms vertex_uniforms{
            .world_camera = world_camera,
            .projection = projection,
        };

        SDL_PushGPUVertexUniformData(command_buffer, 0, &vertex_uniforms, sizeof(VertexUniforms));

        // Setup Fragment Uniforms
        glm::vec4 light_direction_affine = world_camera * glm::vec4(LIGHT_DIRECTION, 0.f);

        FragmentUniforms fragment_uniforms{
            .diffuse_color = DIFFUSE_COLOR,
            .ambient_color = AMBIENT_COLOR,
            .specular_color = SPECULAR_COLOR,
            .specular_exponent = SPECULAR_EXPONENT,
            .light_direction = glm::vec3(light_direction_affine.x, light_direction_affine.y, light_direction_affine.z),
        };

        SDL_PushGPUFragmentUniformData(command_buffer, 0, &fragment_uniforms, sizeof(FragmentUniforms));

        // Render Image
        std::array<SDL_GPUBufferBinding, 1> buffer_bindings{
            SDL_GPUBufferBinding{
                .buffer = state->vertex_buffer,
                .offset = 0,
            }
        };

        SDL_BindGPUVertexBuffers(pass, 0, buffer_bindings.data(), buffer_bindings.size());

        SDL_DrawGPUPrimitives(pass, static_cast<Uint32>(VERTICES.size()), 1, 0, 0);

        SDL_EndGPURenderPass(pass);
    }

    SDL_SubmitGPUCommandBuffer(command_buffer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* app_state, SDL_AppResult result) {
    State* const state = static_cast<State*>(app_state);

    SDL_ReleaseGPUBuffer(state->device, state->vertex_buffer);
    SDL_ReleaseGPUGraphicsPipeline(state->device, state->graphics_pipeline);
    SDL_DestroyGPUDevice(state->device);
    SDL_DestroyWindow(state->window);

    delete state;

    SDL_Quit();
}
