# Render library: GPU resources, shaders, rendering pipeline
add_library(render STATIC
    Window.cpp
    Shader.cpp
    ComputeShader.cpp
    gui/GuiManager.cpp
    gui/ScenePanel.cpp
    gui/TerrainPanel.cpp
    gui/SurfacePanel.cpp
    gui/AtmospherePanel.cpp
    gui/DebugPanel.cpp
    gui/CinematicPanel.cpp
    Renderer.cpp
    Mesh.cpp
    TerrainGenerator.cpp
    GlFence.cpp
    GenerationScheduler.cpp
    CelestialBody.cpp
    Earth.cpp
    BiomePalette.cpp
    GenericBody.cpp
    Framebuffer.cpp
    PostProcessor.cpp
    lod/SpherePatch.cpp
    lod/QuadTreeNode.cpp
    lod/PlanetQuadTree.cpp
    lod/PatchPool.cpp
    effects/OceanRenderer.cpp
    effects/AtmosphereRenderer.cpp
    cinematic/CinematicController.cpp
    cinematic/CaptureManager.cpp
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
    ${STB_INCLUDE_DIR}
)

target_link_libraries(render PUBLIC
    core
    planetgen
    glfw
    OpenGL::GL
    nlohmann_json::nlohmann_json
)
