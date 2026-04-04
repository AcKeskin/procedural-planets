cmake_minimum_required(VERSION 3.20)
project(planetgen VERSION 0.1.0 LANGUAGES CXX)

# ============================================================================
# Shader embedding — convert GLSL sources to C++ string constants
# ============================================================================

set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(EMBEDDED_SHADER_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/EmbeddedShaders.h")

# List shader files for dependency tracking
set(SHADER_SOURCES
    "${SHADER_SOURCE_DIR}/compute/height_earth.comp"
    "${SHADER_SOURCE_DIR}/compute/shading_earth.comp"
    "${SHADER_SOURCE_DIR}/compute/shading_generic.comp"
    "${SHADER_SOURCE_DIR}/compute/erosion_earth.comp"
    "${SHADER_SOURCE_DIR}/includes/noise.glsl"
)

add_custom_command(
    OUTPUT "${EMBEDDED_SHADER_OUTPUT}"
    COMMAND ${CMAKE_COMMAND}
        -DSHADER_DIR="${SHADER_SOURCE_DIR}"
        -DOUTPUT_FILE="${EMBEDDED_SHADER_OUTPUT}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedShaders.cmake"
    DEPENDS ${SHADER_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedShaders.cmake"
    COMMENT "Embedding compute shaders into C++ string constants"
)

add_custom_target(planetgen_embed_shaders DEPENDS "${EMBEDDED_SHADER_OUTPUT}")

# ============================================================================
# Library target
# ============================================================================

set(PLANETGEN_SOURCES
    src/api/planetgen_api.cpp
    src/backend/opengl/OpenGLBackend.cpp
)

set(PLANETGEN_HEADERS
    include/planetgen/planetgen.h
    include/planetgen/planetgen.hpp
    src/api/InternalTypes.h
    src/backend/IComputeBackend.h
    src/backend/opengl/OpenGLBackend.h
)

add_library(planetgen SHARED ${PLANETGEN_SOURCES} ${PLANETGEN_HEADERS})

add_dependencies(planetgen planetgen_embed_shaders)

target_compile_definitions(planetgen PRIVATE PLANETGEN_EXPORTS)

target_include_directories(planetgen
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}  # For generated EmbeddedShaders.h
)

# Map the generated header path so #include "../shaders/EmbeddedShaders.h" resolves
# The API source expects: #include "../shaders/EmbeddedShaders.h"
# Generated file is at: ${CMAKE_CURRENT_BINARY_DIR}/EmbeddedShaders.h
# We'll create a shaders/ subdirectory link via include path
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/shaders")
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/shaders/EmbeddedShaders.h"
    COMMAND ${CMAKE_COMMAND} -E copy "${EMBEDDED_SHADER_OUTPUT}" "${CMAKE_CURRENT_BINARY_DIR}/shaders/EmbeddedShaders.h"
    DEPENDS "${EMBEDDED_SHADER_OUTPUT}"
    COMMENT "Copying embedded shaders header"
)
add_custom_target(planetgen_copy_shaders DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/shaders/EmbeddedShaders.h")
add_dependencies(planetgen planetgen_copy_shaders)

target_link_libraries(planetgen
    PRIVATE
        glfw
        OpenGL::GL
        nlohmann_json::nlohmann_json
)

# gl3w include and source
target_include_directories(planetgen PRIVATE ${GL3W_INCLUDE_DIR})
target_sources(planetgen PRIVATE ${GL3W_SRC})

# ============================================================================
# Install
# ============================================================================

install(TARGETS planetgen
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/planetgen DESTINATION include)
