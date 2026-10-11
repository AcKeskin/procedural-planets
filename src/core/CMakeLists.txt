# Core library: math
add_library(core STATIC
    math/Camera.cpp
    math/CameraController.cpp
)

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(core PUBLIC glm::glm)
