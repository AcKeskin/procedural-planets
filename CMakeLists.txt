cmake_minimum_required(VERSION 3.20)
project(ProceduralPlanets VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Compiler warnings (WX- to allow warnings during development)
if(MSVC)
    add_compile_options(/W4 /WX-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ===== Dependencies =====
include(FetchContent)

# GLFW
message(STATUS "Fetching GLFW...")
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# GLM
message(STATUS "Fetching GLM...")
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

# ImGui (docking branch)
message(STATUS "Fetching ImGui...")
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)
set(IMGUI_DIR ${imgui_SOURCE_DIR})

# gl3w
message(STATUS "Fetching gl3w...")
FetchContent_Declare(
    gl3w
    GIT_REPOSITORY https://github.com/skaslev/gl3w.git
    GIT_TAG master
)
FetchContent_MakeAvailable(gl3w)

# stb (single-header libraries)
message(STATUS "Fetching stb...")
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)
set(STB_INCLUDE_DIR ${stb_SOURCE_DIR})

# nlohmann/json (header-only)
message(STATUS "Fetching nlohmann/json...")
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

# gl3w generates files in build dir, not source dir
set(GL3W_INCLUDE_DIR ${gl3w_BINARY_DIR}/include)
set(GL3W_SRC ${gl3w_BINARY_DIR}/src/gl3w.c)

# OpenGL
find_package(OpenGL REQUIRED)

# Subdirectories
add_subdirectory(src/core)
add_subdirectory(libplanetgen)
add_subdirectory(src/render)
add_subdirectory(src/app)
