#include "Input.h"
#include "Window.h"
#include "Gui.h"
#include "Renderer.h"
#include "Shader.h"
#include "Mesh.h"
#include "MeshGenerator.h"
#include "TerrainGenerator.h"
#include "lod/PatchLodSystem.h"
#include "effects/OceanRenderer.h"
#include "effects/AtmosphereRenderer.h"
#include "math/Camera.h"
#include "generation/Planet.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>

int main()
{
    planets::render::Window window;
    planets::render::WindowConfig windowConfig;
    windowConfig.title = "Procedural Planets";

    if (!window.Initialize(windowConfig))
    {
        return -1;
    }

    planets::app::Input input;
    input.Initialize(window.GetHandle());

    planets::render::Renderer renderer;
    renderer.Initialize();

    planets::render::Gui gui;
    gui.Initialize(window.GetHandle());

    // Load planet shader
    planets::render::Shader planetShader;
    if (!planetShader.LoadFromFiles("shaders/planet.vert", "shaders/planet.frag"))
    {
        std::cerr << "Failed to load planet shader" << std::endl;
        return -1;
    }

    // Initialize GPU terrain generator
    planets::render::TerrainGenerator terrainGenerator;
    bool useGpu = terrainGenerator.Initialize("shaders/compute/height_earth.comp");
    if (useGpu)
    {
        std::cout << "[TerrainGenerator] GPU compute shader loaded successfully" << std::endl;
    }
    else
    {
        std::cout << "[TerrainGenerator] GPU compute shader not available, using CPU" << std::endl;
    }

    // Planet settings
    float planetRadius = 10.0f;  // Larger planet for LOD system
    float seaLevel = 0.4f;
    int subdivisions = 6;
    uint32_t seed = 42;

    // LOD system settings
    bool useLodSystem = true;
    int patchSubdivisions = 2;  // 0=20, 1=80, 2=320 patches

    // Earth terrain settings with realistic defaults
    planets::render::EarthTerrainSettings terrainSettings;

    // LOD patch system
    planets::render::lod::PatchLodSystem lodSystem;

    // Ocean rendering
    planets::render::effects::OceanRenderer oceanRenderer;
    planets::render::effects::OceanSettings oceanSettings;

    // Atmosphere rendering
    planets::render::effects::AtmosphereRenderer atmosphereRenderer;
    planets::render::effects::AtmosphereSettings atmosphereSettings;

    // For CPU fallback, still need Planet object
    planets::core::PlanetSettings planetSettings;
    planetSettings.radius = planetRadius;
    planetSettings.seaLevel = seaLevel;
    planetSettings.subdivisions = subdivisions;
    planetSettings.seed = seed;
    planets::core::Planet planet(planetSettings);

    // Generation timing
    float lastCpuTimeMs = 0.0f;
    float lastGpuTimeMs = 0.0f;

    // Generate mesh
    planets::render::Mesh planetMesh;

    auto regeneratePlanetCpu = [&]() {
        auto start = std::chrono::high_resolution_clock::now();

        // Update planet settings for CPU path
        planetSettings.subdivisions = subdivisions;
        planetSettings.seed = seed;
        planet.Configure(planetSettings);
        planet.Reseed(seed);

        auto meshData = planets::render::MeshGenerator::GeneratePlanetMesh(planet, subdivisions);
        planetMesh.Upload(meshData);

        auto end = std::chrono::high_resolution_clock::now();
        lastCpuTimeMs = std::chrono::duration<float, std::milli>(end - start).count();

        std::cout << "[Planet] CPU generated " << planetMesh.GetVertexCount()
                  << " vertices in " << lastCpuTimeMs << " ms" << std::endl;
    };

    auto regeneratePlanetGpu = [&]() {
        if (!terrainGenerator.IsReady())
        {
            regeneratePlanetCpu();
            return;
        }

        auto start = std::chrono::high_resolution_clock::now();

        // Get sphere vertices for compute shader
        auto vertices = planets::render::MeshGenerator::GetIcosphereVertices(subdivisions);

        // Generate heights using Earth terrain settings
        auto heights = terrainGenerator.GenerateHeights(vertices, seed, terrainSettings);

        // Apply heights to mesh
        auto meshData = planets::render::MeshGenerator::GeneratePlanetMesh(
            subdivisions, planetRadius, heights);
        planetMesh.Upload(meshData);

        auto end = std::chrono::high_resolution_clock::now();
        lastGpuTimeMs = std::chrono::duration<float, std::milli>(end - start).count();

        std::cout << "[Planet] GPU generated " << planetMesh.GetVertexCount()
                  << " vertices in " << lastGpuTimeMs << " ms" << std::endl;
    };

    // LOD system regeneration
    auto regenerateLodSystem = [&]() {
        if (!terrainGenerator.IsReady())
        {
            std::cerr << "[LOD] GPU compute shader required for LOD system" << std::endl;
            return;
        }

        auto start = std::chrono::high_resolution_clock::now();

        lodSystem.Initialize(planetRadius, patchSubdivisions, terrainGenerator, terrainSettings, seed);

        // Initialize ocean at sea level
        oceanRenderer.Initialize(planetRadius, seaLevel, 5);

        // Initialize atmosphere
        atmosphereRenderer.Initialize(planetRadius, 4);

        auto end = std::chrono::high_resolution_clock::now();
        lastGpuTimeMs = std::chrono::duration<float, std::milli>(end - start).count();

        std::cout << "[LOD] Generated " << lodSystem.GetPatchCount()
                  << " patches in " << lastGpuTimeMs << " ms" << std::endl;
    };

    // Initial generation
    if (useLodSystem && useGpu)
    {
        regenerateLodSystem();
    }
    else if (useGpu)
    {
        regeneratePlanetGpu();
    }
    else
    {
        regeneratePlanetCpu();
    }

    planets::core::Camera camera;
    camera.SetPosition(glm::vec3(0.0f, 0.0f, 25.0f));  // Farther for larger planet

    float lastTime = static_cast<float>(glfwGetTime());
    float moveSpeed = 15.0f;  // Adjustable via GUI
    const float lookSpeed = 0.1f;

    // Light direction
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));

    while (!window.ShouldClose())
    {
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        window.PollEvents();
        input.Update();

        // Exit on ESC
        if (input.IsKeyDown(planets::app::Key::Escape))
        {
            window.SetShouldClose(true);
        }

        // Toggle cursor capture with right mouse button
        if (input.IsMouseButtonDown(planets::app::MouseButton::Right))
        {
            if (input.IsCursorEnabled())
            {
                input.SetCursorEnabled(false);
            }

            camera.Rotate(input.GetMouseDeltaX() * lookSpeed, input.GetMouseDeltaY() * lookSpeed);
        }
        else
        {
            if (!input.IsCursorEnabled())
            {
                input.SetCursorEnabled(true);
            }
        }

        // Camera movement with shift boost
        float speedMultiplier = input.IsKeyDown(planets::app::Key::LeftShift) ? 5.0f : 1.0f;
        float speed = moveSpeed * deltaTime * speedMultiplier;
        if (input.IsKeyDown(planets::app::Key::W))
            camera.MoveForward(speed);
        if (input.IsKeyDown(planets::app::Key::S))
            camera.MoveForward(-speed);
        if (input.IsKeyDown(planets::app::Key::A))
            camera.MoveRight(-speed);
        if (input.IsKeyDown(planets::app::Key::D))
            camera.MoveRight(speed);
        if (input.IsKeyDown(planets::app::Key::E))
            camera.MoveUp(speed);
        if (input.IsKeyDown(planets::app::Key::Q))
            camera.MoveUp(-speed);

        // Render
        renderer.BeginFrame();

        // Draw planet
        planetShader.Use();

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = camera.GetProjectionMatrix(window.GetAspectRatio());
        glm::mat4 viewProjection = projection * view;

        planetShader.SetMat4("uModel", model);
        planetShader.SetMat4("uView", view);
        planetShader.SetMat4("uProjection", projection);
        planetShader.SetVec3("uLightDir", lightDir);
        planetShader.SetVec3("uCameraPos", camera.GetPosition());
        planetShader.SetFloat("uSeaLevel", seaLevel);

        if (useLodSystem && useGpu)
        {
            lodSystem.UpdateLods(camera.GetPosition(), viewProjection);
            lodSystem.Render(planetShader);

            // Render ocean
            oceanRenderer.Render(view, projection, camera.GetPosition(), lightDir, oceanSettings);

            // Render atmosphere (last, additive blend)
            atmosphereRenderer.Render(view, projection, camera.GetPosition(), lightDir,
                                      planetRadius, atmosphereSettings);
        }
        else
        {
            planetMesh.Draw();
        }

        // GUI
        gui.BeginFrame();
        gui.DrawDebugPanel(camera, moveSpeed);

        // GPU toggle and timing display
        gui.DrawGpuPanel(useGpu, lastCpuTimeMs, lastGpuTimeMs, terrainGenerator.IsReady());

        // LOD System panel
        bool lodNeedsRegen = gui.DrawLodPanel(
            useLodSystem,
            patchSubdivisions,
            planetRadius,
            lodSystem.GetPatchCount(),
            lodSystem.GetVisiblePatchCount(),
            lodSystem.GetTotalVertexCount());

        // Ocean panel
        gui.DrawOceanPanel(oceanSettings, seaLevel);

        // Atmosphere panel
        gui.DrawAtmospherePanel(atmosphereSettings);

        // Earth terrain panel (GPU mode) or Planet panel (CPU mode)
        bool needsRegen = false;
        if (useGpu)
        {
            needsRegen = gui.DrawEarthTerrainPanel(terrainSettings, seed, subdivisions);
        }
        else
        {
            needsRegen = gui.DrawPlanetPanel(planet);
            subdivisions = planet.GetSettings().subdivisions;
            seed = planet.GetSettings().seed;
        }

        if (needsRegen || lodNeedsRegen)
        {
            if (useLodSystem && useGpu)
            {
                regenerateLodSystem();
            }
            else if (useGpu)
            {
                regeneratePlanetGpu();
            }
            else
            {
                regeneratePlanetCpu();
            }
        }
        gui.EndFrame();

        renderer.EndFrame();
        window.SwapBuffers();
    }

    gui.Shutdown();
    renderer.Shutdown();
    window.Shutdown();

    return 0;
}
