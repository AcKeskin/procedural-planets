# Render library: GPU resources, shaders, rendering pipeline
add_library(render STATIC
    Window.cpp
    Shader.cpp
    gui/GuiManager.cpp
    gui/ScenePanel.cpp
    gui/TerrainPanel.cpp
    gui/SurfacePanel.cpp
    gui/AtmospherePanel.cpp
    gui/DebugPanel.cpp
    gui/CinematicPanel.cpp
    gui/CloudsPanel.cpp
    Renderer.cpp
    GpuTimer.cpp
    Mesh.cpp
    GlFence.cpp
    GenerationScheduler.cpp
    BodyRuntime.cpp
    BiomePalette.cpp
    Framebuffer.cpp
    PostProcessor.cpp
    lod/SpherePatch.cpp
    lod/QuadTreeNode.cpp
    lod/PlanetQuadTree.cpp
    lod/PatchPool.cpp
    lod/SharedPatchTopology.cpp
    effects/OceanRenderer.cpp
    effects/AtmosphereRenderer.cpp
    effects/CloudVolume.cpp
    effects/BloomPass.cpp
    effects/GroundTextures.cpp
    materials/GroundMaterials.cpp
    materials/CloudNoise.cpp
    cinematic/CinematicController.cpp
    cinematic/CaptureManager.cpp
    types/BodyRegistry.cpp
    types/BodyLook.cpp
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

# Tests for the render-side data code (planet type registry and traits). Runs from the source
# root so it can read data/.
add_executable(render_tests
    tests/test_body_registry.cpp
    tests/test_body_look.cpp
    tests/test_ground_materials.cpp
    tests/test_cloud_noise.cpp
)
target_link_libraries(render_tests PRIVATE render Catch2::Catch2WithMain)
catch_discover_tests(render_tests WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
