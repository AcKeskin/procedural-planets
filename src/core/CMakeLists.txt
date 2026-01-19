# Core library: math, noise, generation algorithms
add_library(core STATIC
    math/Camera.cpp
    noise/SimplexNoise.cpp
    noise/FractalNoise.cpp
    generation/NoiseLayer.cpp
    generation/Planet.cpp
)

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(core PUBLIC glm::glm)
