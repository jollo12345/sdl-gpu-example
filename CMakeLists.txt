cmake_minimum_required(VERSION 3.16)

project(sdl_gpu_example LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
  _sdl
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.4.0)

FetchContent_Declare(
  _jml
  GIT_REPOSITORY https://github.com/jollo12345/jollos-math-library.git
  GIT_TAG main)

FetchContent_MakeAvailable(_sdl _jml)

set(${PROJECT_NAME}_SHADERS phong.vert phong.frag)

add_executable(${PROJECT_NAME} main.c)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 JML::JML)

set(${PROJECT_NAME}_compiled_shaders)

foreach(${PROJECT_NAME}_SHADER ${${PROJECT_NAME}_SHADERS})
  set(SHADER_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${${PROJECT_NAME}_SHADER}.spv")

  add_custom_command(
    OUTPUT "${SHADER_OUTPUT}"
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/assets"
    COMMAND glslc "${CMAKE_CURRENT_SOURCE_DIR}/${${PROJECT_NAME}_SHADER}" -o
            "${SHADER_OUTPUT}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${${PROJECT_NAME}_SHADER}"
    COMMENT "Compiling Shader: \"${${PROJECT_NAME}_SHADER}\"")

  list(APPEND ${PROJECT_NAME}_compiled_shaders "${SHADER_OUTPUT}")
endforeach()

add_custom_target(${PROJECT_NAME}_shaders ALL
                  DEPENDS ${${PROJECT_NAME}_compiled_shaders})

add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_shaders)
