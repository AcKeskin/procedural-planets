# Core library: math, noise
add_library(core STATIC
    math/Camera.cpp
    noise/SimplexNoise.cpp
    noise/FractalNoise.cpp
)

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(core PUBLIC glm::glm)
