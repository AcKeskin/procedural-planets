# Core library: math, noise, generation algorithms
add_library(core STATIC
    math/Camera.cpp
)

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(core PUBLIC glm::glm)
