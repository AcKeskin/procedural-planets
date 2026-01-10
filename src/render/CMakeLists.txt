# Render library: GPU resources, shaders, rendering pipeline
add_library(render STATIC
    Window.cpp
    Shader.cpp
    Gui.cpp
    Renderer.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${GL3W_SRC}
)

target_include_directories(render PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${GL3W_INCLUDE_DIR}
)

target_link_libraries(render PUBLIC
    core
    glfw
    OpenGL::GL
)
