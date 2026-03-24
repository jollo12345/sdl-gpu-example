# SDL GPU Example

A simple single file example demonstrating the SDL3 GPU API implementing a Phong
Shading Pipeline. Shaders are written in GLSL.

![image](example.png)

# Build Requirements

The following tools must be installed on your system before building:

- CMake 3.16 or newer
- A C++20 compatible compiler
- glslc (included in the Vulkan SDK)

SDL and GLM are downloaded automatically via 'FetchContent'.
