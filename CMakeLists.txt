cmake_minimum_required(VERSION 3.16)

project(sdl_gpu_example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    sdl
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.0
)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.3
)

FetchContent_MakeAvailable(
    sdl
    glm
)

set(${PROJECT_NAME}_SHADERS
    phong.vert
    phong.frag
)

add_executable(${PROJECT_NAME}
    main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    glm::glm
)

set(${PROJECT_NAME}_compiled_shaders)

foreach(${PROJECT_NAME}_SHADER ${${PROJECT_NAME}_SHADERS})
    set(SHADER_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${${PROJECT_NAME}_SHADER}.spv")

    add_custom_command(
        OUTPUT "${SHADER_OUTPUT}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/assets"
        COMMAND glslc "${CMAKE_CURRENT_SOURCE_DIR}/${${PROJECT_NAME}_SHADER}" -o "${SHADER_OUTPUT}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${${PROJECT_NAME}_SHADER}"
        COMMENT "Compiling Shader: \"${${PROJECT_NAME}_SHADER}\""
    )

    list(APPEND ${PROJECT_NAME}_compiled_shaders "${SHADER_OUTPUT}")
endforeach()

add_custom_target(${PROJECT_NAME}_shaders ALL DEPENDS
    ${${PROJECT_NAME}_compiled_shaders}
)

add_dependencies(${PROJECT_NAME}
    ${PROJECT_NAME}_shaders
)
