#include "Input.h"
#include "Window.h"
#include "Gui.h"
#include "Renderer.h"
#include "Shader.h"
#include "Mesh.h"
#include "MeshGenerator.h"
#include "TerrainGenerator.h"
#include "Framebuffer.h"
#include "PostProcessor.h"
#include "lod/PatchLodSystem.h"
#include "effects/OceanRenderer.h"
#include "effects/AtmosphereRenderer.h"
#include "math/Camera.h"
#include "generation/Planet.h"
#include <GL/gl3w.h>
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

    // Initialize framebuffer for off-screen rendering
    planets::render::Framebuffer sceneFbo;
    if (!sceneFbo.Create(window.GetWidth(), window.GetHeight()))
    {
        std::cerr << "Failed to create scene framebuffer" << std::endl;
        return -1;
    }

    // Initialize post-processor for fullscreen blitting
    planets::render::PostProcessor postProcessor;
    if (!postProcessor.Initialize())
    {
        std::cerr << "Failed to initialize post-processor" << std::endl;
        return -1;
    }

    // Load passthrough shader for blitting scene to screen
    planets::render::Shader passthroughShader;
    if (!passthroughShader.LoadFromFiles("shaders/passthrough.vert", "shaders/passthrough.frag"))
    {
        std::cerr << "Failed to load passthrough shader" << std::endl;
        return -1;
    }

    // Load planet shader
    planets::render::Shader planetShader;
    if (!planetShader.LoadFromFiles("shaders/planet.vert", "shaders/planet.frag"))
    {
        std::cerr << "Failed to load planet shader" << std::endl;
        return -1;
    }

    // Load space background shader
    planets::render::Shader spaceShader;
    if (!spaceShader.LoadFromFiles("shaders/space.vert", "shaders/space.frag"))
    {
        std::cerr << "Failed to load space shader" << std::endl;
        return -1;
    }

    // Initialize GPU terrain generator with both height and shading shaders
    planets::render::TerrainGenerator terrainGenerator;
    bool useGpu = terrainGenerator.Initialize(
        "shaders/compute/height_earth.comp",
        "shaders/compute/shading_earth.comp");
    if (useGpu)
    {
        std::cout << "[TerrainGenerator] GPU compute shaders loaded successfully" << std::endl;
    }
    else
    {
        std::cout << "[TerrainGenerator] GPU compute shaders not available, using CPU" << std::endl;
    }

    // Planet settings
    float planetRadius = 10.0f;  // Larger planet for LOD system
    float seaLevel = 0.995f;     // Sea level as height multiplier (terrain is ~0.96-1.04)
    int subdivisions = 6;
    uint32_t seed = 42;

    // LOD system settings
    bool useLodSystem = true;
    int patchSubdivisions = 2;  // 0=20, 1=80, 2=320 patches

    // Earth terrain settings with realistic defaults
    planets::render::EarthTerrainSettings terrainSettings;

    // Earth shading settings
    planets::render::EarthShadingSettings shadingSettings;

    // LOD patch system
    planets::render::lod::PatchLodSystem lodSystem;

    // Ocean rendering
    planets::render::effects::OceanRenderer oceanRenderer;
    planets::render::effects::OceanSettings oceanSettings;

    // Atmosphere rendering
    planets::render::effects::AtmosphereRenderer atmosphereRenderer;
    planets::render::effects::AtmosphereSettings atmosphereSettings;

    // Biome settings for shader
    planets::render::BiomeSettings biomeSettings;

    // Earth colors (gradient pairs)
    planets::render::EarthColors earthColors;

    // Lighting settings
    planets::render::Gui::LightingSettings lightingSettings;

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

        // Generate shading data if shader is available
        planets::render::MeshData meshData;
        if (terrainGenerator.IsShadingReady())
        {
            auto shadingData = terrainGenerator.GenerateShadingData(vertices, seed, shadingSettings);
            meshData = planets::render::MeshGenerator::GeneratePlanetMesh(
                subdivisions, planetRadius, heights, shadingData);
        }
        else
        {
            // Fallback to mesh without shading data
            meshData = planets::render::MeshGenerator::GeneratePlanetMesh(
                subdivisions, planetRadius, heights);
        }

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

        lodSystem.Initialize(planetRadius, patchSubdivisions, terrainGenerator, terrainSettings, shadingSettings, seed);

        // Initialize ocean at sea level
        oceanRenderer.Initialize(planetRadius, seaLevel, 5);

        // Initialize atmosphere (post-process)
        atmosphereRenderer.Initialize();

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

    // Track window dimensions for framebuffer resize
    int previousWidth = window.GetWidth();
    int previousHeight = window.GetHeight();

    // Light direction (sun direction)
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));

    // Sun settings for space background
    float sunSize = 0.03f;  // Angular size in radians (~1.7 degrees)
    glm::vec3 sunColor = glm::vec3(1.0f, 0.95f, 0.8f);  // Warm white
    float starDensity = 1.0f;

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

        // Handle window resize - recreate framebuffer if dimensions changed
        int currentWidth = window.GetWidth();
        int currentHeight = window.GetHeight();
        if (currentWidth != previousWidth || currentHeight != previousHeight)
        {
            sceneFbo.Resize(currentWidth, currentHeight);
            previousWidth = currentWidth;
            previousHeight = currentHeight;
        }

        // Phase 1: Render scene to framebuffer
        sceneFbo.Bind();
        renderer.BeginFrame();  // Clear color and depth

        // Calculate matrices
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = camera.GetProjectionMatrix(window.GetAspectRatio());
        glm::mat4 viewProjection = projection * view;
        glm::mat4 invView = glm::inverse(view);
        glm::mat4 invProjection = glm::inverse(projection);

        // Draw space background (stars and sun) first
        glDepthMask(GL_FALSE);  // Don't write to depth buffer
        spaceShader.Use();
        spaceShader.SetMat4("uInvProjection", invProjection);
        spaceShader.SetMat4("uInvView", invView);
        spaceShader.SetVec3("uLightDir", lightDir);
        spaceShader.SetFloat("uSunSize", sunSize);
        spaceShader.SetVec3("uSunColor", sunColor);
        spaceShader.SetFloat("uStarDensity", starDensity);
        postProcessor.RenderQuad();
        glDepthMask(GL_TRUE);  // Re-enable depth writing

        // Draw planet
        planetShader.Use();

        planetShader.SetMat4("uModel", model);
        planetShader.SetMat4("uView", view);
        planetShader.SetMat4("uProjection", projection);
        planetShader.SetVec3("uLightDir", lightDir);
        planetShader.SetVec3("uCameraPos", camera.GetPosition());
        planetShader.SetFloat("uSeaLevel", seaLevel);

        // Biome parameters for shading
        planetShader.SetInt("uUseBiomes", biomeSettings.enabled ? 1 : 0);
        planetShader.SetFloat("uSteepnessThreshold", biomeSettings.steepnessThreshold);
        planetShader.SetFloat("uFlatToSteepBlend", biomeSettings.flatToSteepBlend);
        planetShader.SetFloat("uSnowLatitude", biomeSettings.snowLatitude);
        planetShader.SetFloat("uSnowBlend", biomeSettings.snowBlend);
        planetShader.SetFloat("uSnowLine", biomeSettings.snowLine);
        planetShader.SetFloat("uShoreHeight", biomeSettings.shoreHeight);

        // Earth color uniforms (gradient pairs)
        planetShader.SetVec3("uShoreLow", earthColors.shoreLow);
        planetShader.SetVec3("uShoreHigh", earthColors.shoreHigh);
        planetShader.SetVec3("uFlatLowA", earthColors.flatLowA);
        planetShader.SetVec3("uFlatHighA", earthColors.flatHighA);
        planetShader.SetVec3("uFlatLowB", earthColors.flatLowB);
        planetShader.SetVec3("uFlatHighB", earthColors.flatHighB);
        planetShader.SetVec3("uSteepLow", earthColors.steepLow);
        planetShader.SetVec3("uSteepHigh", earthColors.steepHigh);
        planetShader.SetVec3("uSnowColor", earthColors.snow);

        // Color blending parameters
        planetShader.SetFloat("uFlatColBlend", shadingSettings.flatColBlend);
        planetShader.SetFloat("uFlatColBlendNoise", shadingSettings.flatColBlendNoise);

        // Height range for normalization
        float heightMin, heightMax;
        if (biomeSettings.autoHeightRange)
        {
            // Estimate from terrain settings - heights are ~[1.0 - heightScale, 1.0 + heightScale]
            heightMin = 1.0f - terrainSettings.heightScale * 1.5f;
            heightMax = 1.0f + terrainSettings.heightScale * 1.5f;
        }
        else
        {
            // Manual override for fine-tuning
            heightMin = biomeSettings.heightRangeMin;
            heightMax = biomeSettings.heightRangeMax;
        }
        planetShader.SetVec2("uHeightMinMax", glm::vec2(heightMin, heightMax));

        // Lighting parameters
        planetShader.SetFloat("uSunIntensity", lightingSettings.sunIntensity);
        planetShader.SetFloat("uAmbientLight", lightingSettings.ambientLight);
        planetShader.SetFloat("uSpecularStrength", lightingSettings.specularStrength);
        planetShader.SetFloat("uSpecularPower", lightingSettings.specularPower);

        if (useLodSystem && useGpu)
        {
            lodSystem.UpdateLods(camera.GetPosition(), viewProjection);
            lodSystem.Render(planetShader);

            // Render ocean
            oceanRenderer.Render(view, projection, camera.GetPosition(), lightDir, oceanSettings);
        }
        else
        {
            planetMesh.Draw();
        }

        sceneFbo.Unbind();

        // Phase 2: Apply atmosphere post-process to default framebuffer
        renderer.BeginFrame();  // Clear default framebuffer

        // Calculate ocean radius for atmosphere ray termination
        float oceanRadius = planetRadius * seaLevel;
        glm::vec3 planetCenter = glm::vec3(0.0f);  // Planet at origin

        // Render atmosphere as post-process (includes scene compositing)
        if (atmosphereSettings.enabled)
        {
            atmosphereRenderer.Render(
                view, projection,
                invView, invProjection,
                camera.GetPosition(), lightDir,
                planetCenter, planetRadius, oceanRadius,
                0.1f, 1000.0f,  // Near/far planes (match camera)
                atmosphereSettings,
                sceneFbo.GetColorTexture(),
                sceneFbo.GetDepthTexture());
        }
        else
        {
            // Passthrough when atmosphere disabled
            passthroughShader.Use();
            passthroughShader.SetInt("uSceneTexture", 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sceneFbo.GetColorTexture());
            postProcessor.RenderQuad();
        }

        // GUI
        gui.BeginFrame();

        gui.DrawScenePanel(lightDir, sunSize, sunColor, starDensity, lightingSettings);

        bool needsRegen = gui.DrawTerrainPanel(
            useGpu,
            terrainSettings, seed, subdivisions,
            planet,
            useLodSystem, patchSubdivisions, planetRadius,
            lodSystem.GetPatchCount(),
            lodSystem.GetVisiblePatchCount(),
            lodSystem.GetTotalVertexCount(),
            lastCpuTimeMs, lastGpuTimeMs, terrainGenerator.IsReady());

        if (!useGpu)
        {
            subdivisions = planet.GetSettings().subdivisions;
            seed = planet.GetSettings().seed;
        }

        gui.DrawSurfacePanel(biomeSettings, earthColors, shadingSettings, oceanSettings, seaLevel);
        gui.DrawAtmospherePanel(atmosphereSettings);
        gui.DrawDebugPanel(camera, moveSpeed);

        if (needsRegen)
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

    postProcessor.Shutdown();
    gui.Shutdown();
    renderer.Shutdown();
    window.Shutdown();

    return 0;
}
