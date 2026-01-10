# Core library: math, noise, generation algorithms
add_library(core STATIC
    Core.cpp
)

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
