#include "Input.h"
#include "Window.h"
#include "Gui.h"
#include "Renderer.h"
#include "Shader.h"
#include "Mesh.h"
#include "MeshGenerator.h"
#include "TerrainGenerator.h"
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

    // Create planet with higher quality defaults
    planets::core::PlanetSettings planetSettings;
    planetSettings.radius = 2.0f;
    planetSettings.seaLevel = 0.4f;
    planetSettings.heightScale = 1.0f;  // Increased for visible terrain
    planetSettings.subdivisions = 6;     // Higher quality (40k vertices)
    planetSettings.seed = 42;

    planets::core::Planet planet(planetSettings);

    // Generation timing
    float lastCpuTimeMs = 0.0f;
    float lastGpuTimeMs = 0.0f;

    // Generate mesh
    planets::render::Mesh planetMesh;

    auto regeneratePlanetCpu = [&]() {
        auto start = std::chrono::high_resolution_clock::now();

        auto meshData = planets::render::MeshGenerator::GeneratePlanetMesh(
            planet, planet.GetSettings().subdivisions);
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
        auto vertices = planets::render::MeshGenerator::GetIcosphereVertices(
            planet.GetSettings().subdivisions);

        // Use Planet's first noise layer for GPU parameters
        const auto& layer0 = planet.GetNoiseLayer(0).GetSettings();
        float heightMult = planet.GetSettings().heightScale;

        planets::render::ComputeNoiseParams continentParams;
        continentParams.offset = glm::vec3(0.0f);
        continentParams.numLayers = layer0.octaves;
        continentParams.scale = layer0.scale;
        continentParams.persistence = layer0.persistence;
        continentParams.lacunarity = layer0.lacunarity;
        continentParams.multiplier = layer0.strength * heightMult;

        // Mountain params (higher frequency detail)
        planets::render::ComputeNoiseParams mountainParams;
        mountainParams.offset = glm::vec3(1000.0f);
        mountainParams.numLayers = layer0.octaves + 1;
        mountainParams.scale = layer0.scale * 2.0f;
        mountainParams.persistence = layer0.persistence;
        mountainParams.lacunarity = layer0.lacunarity;
        mountainParams.multiplier = layer0.strength * heightMult;
        mountainParams.power = 2.0f;
        mountainParams.gain = 1.0f;
        mountainParams.smoothOffset = 0.3f;

        // Mask params
        planets::render::ComputeNoiseParams maskParams;
        maskParams.offset = glm::vec3(2000.0f);
        maskParams.numLayers = 4;
        maskParams.scale = layer0.scale * 0.75f;
        maskParams.persistence = 0.5f;
        maskParams.lacunarity = 2.0f;
        maskParams.multiplier = 1.0f;

        // Generate heights on GPU (Solar-System reference values)
        auto heights = terrainGenerator.GenerateHeights(
            vertices,
            planet.GetSettings().seed,
            continentParams,
            mountainParams,
            maskParams,
            5.0f,   // oceanDepthMultiplier (exaggerate ocean depth)
            1.5f,   // oceanFloorDepth (minimum ocean depth)
            0.5f,   // oceanFloorSmoothing (transition zone)
            1.2f    // mountainBlend (elevation-based blend zone)
        );

        // Apply heights to mesh
        auto meshData = planets::render::MeshGenerator::GeneratePlanetMesh(
            planet.GetSettings().subdivisions,
            planet.GetSettings().radius,
            heights);
        planetMesh.Upload(meshData);

        auto end = std::chrono::high_resolution_clock::now();
        lastGpuTimeMs = std::chrono::duration<float, std::milli>(end - start).count();

        std::cout << "[Planet] GPU generated " << planetMesh.GetVertexCount()
                  << " vertices in " << lastGpuTimeMs << " ms" << std::endl;
    };

    // Initial generation
    if (useGpu)
    {
        regeneratePlanetGpu();
    }
    else
    {
        regeneratePlanetCpu();
    }

    planets::core::Camera camera;
    camera.SetPosition(glm::vec3(0.0f, 0.0f, 6.0f));

    float lastTime = static_cast<float>(glfwGetTime());
    const float moveSpeed = 5.0f;
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

        // Camera movement
        float speed = moveSpeed * deltaTime;
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

        planetShader.SetMat4("uModel", model);
        planetShader.SetMat4("uView", view);
        planetShader.SetMat4("uProjection", projection);
        planetShader.SetVec3("uLightDir", lightDir);
        planetShader.SetVec3("uCameraPos", camera.GetPosition());
        planetShader.SetFloat("uSeaLevel", planet.GetSettings().seaLevel);

        planetMesh.Draw();

        // GUI
        gui.BeginFrame();
        gui.DrawDebugPanel(camera);

        // GPU toggle and timing display
        gui.DrawGpuPanel(useGpu, lastCpuTimeMs, lastGpuTimeMs, terrainGenerator.IsReady());

        if (gui.DrawPlanetPanel(planet))
        {
            if (useGpu)
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
