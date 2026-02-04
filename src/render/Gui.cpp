#include "Gui.h"
#include "TerrainGenerator.h"
#include "effects/OceanRenderer.h"
#include "effects/AtmosphereRenderer.h"
#include "../core/math/Camera.h"
#include "../core/generation/Planet.h"
#include "../core/generation/NoiseLayer.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <string>
#include <cstdlib>

namespace planets::render {

Gui::Gui()
    : _initialized(false)
{
}

Gui::~Gui()
{
    Shutdown();
}

bool Gui::Initialize(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");

    _initialized = true;
    return true;
}

void Gui::Shutdown()
{
    if (_initialized)
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        _initialized = false;
    }
}

void Gui::BeginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Gui::EndFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Gui::DrawDebugPanel(const planets::core::Camera& camera, float& moveSpeed)
{
    ImGui::Begin("Debug");

    const auto& pos = camera.GetPosition();
    ImGui::Text("Camera Position: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
    ImGui::Text("Yaw: %.2f  Pitch: %.2f", camera.GetYaw(), camera.GetPitch());

    const char* modeStr = camera.GetMode() == planets::core::CameraMode::FreeFly ? "Free-Fly" : "Orbit";
    ImGui::Text("Mode: %s", modeStr);

    ImGui::Separator();
    ImGui::Text("Movement");
    ImGui::SliderFloat("Speed", &moveSpeed, 1.0f, 100.0f, "%.1f");

    ImGui::Separator();
    ImGui::Text("Controls:");
    ImGui::BulletText("WASD - Move");
    ImGui::BulletText("Mouse (RMB) - Look");
    ImGui::BulletText("Q/E - Up/Down");
    ImGui::BulletText("Shift - Boost");
    ImGui::BulletText("ESC - Exit");

    ImGui::End();
}

bool Gui::DrawPlanetPanel(planets::core::Planet& planet)
{
    bool needsRegeneration = false;

    ImGui::Begin("Planet");

    auto& settings = planet.GetSettings();

    ImGui::Text("Planet Settings");
    ImGui::Separator();

    if (ImGui::SliderFloat("Radius", &settings.radius, 0.5f, 5.0f))
    {
        needsRegeneration = true;
    }

    if (ImGui::SliderFloat("Sea Level", &settings.seaLevel, 0.0f, 1.0f))
    {
        // Sea level doesn't require mesh regeneration, just shader uniform update
    }

    if (ImGui::SliderFloat("Height Scale", &settings.heightScale, 0.0f, 0.2f))
    {
        needsRegeneration = true;
    }

    if (ImGui::SliderInt("Subdivisions", &settings.subdivisions, 1, 6))
    {
        needsRegeneration = true;
    }

    int seed = static_cast<int>(settings.seed);
    if (ImGui::InputInt("Seed", &seed))
    {
        settings.seed = static_cast<uint32_t>(seed);
        planet.Reseed(settings.seed);
        needsRegeneration = true;
    }

    ImGui::Separator();
    ImGui::Text("Noise Layers: %zu", planet.GetNoiseLayerCount());

    for (size_t i = 0; i < planet.GetNoiseLayerCount(); ++i)
    {
        auto& layer = planet.GetNoiseLayer(i);
        auto& layerSettings = layer.GetSettings();

        ImGui::PushID(static_cast<int>(i));

        if (ImGui::CollapsingHeader(("Layer " + std::to_string(i)).c_str()))
        {
            if (ImGui::Checkbox("Enabled", &layerSettings.enabled))
            {
                layer.Configure(layerSettings);
                needsRegeneration = true;
            }

            if (ImGui::SliderFloat("Scale", &layerSettings.scale, 0.1f, 10.0f))
            {
                layer.Configure(layerSettings);
                needsRegeneration = true;
            }

            if (ImGui::SliderFloat("Strength", &layerSettings.strength, 0.0f, 2.0f))
            {
                layer.Configure(layerSettings);
                needsRegeneration = true;
            }

            if (ImGui::SliderInt("Octaves", &layerSettings.octaves, 1, 8))
            {
                layer.Configure(layerSettings);
                needsRegeneration = true;
            }

            if (ImGui::SliderFloat("Persistence", &layerSettings.persistence, 0.1f, 1.0f))
            {
                layer.Configure(layerSettings);
                needsRegeneration = true;
            }

            if (ImGui::SliderFloat("Lacunarity", &layerSettings.lacunarity, 1.0f, 4.0f))
            {
                layer.Configure(layerSettings);
                needsRegeneration = true;
            }
        }

        ImGui::PopID();
    }

    if (ImGui::Button("Regenerate"))
    {
        needsRegeneration = true;
    }

    ImGui::End();

    return needsRegeneration;
}

void Gui::DrawGpuPanel(bool& useGpu, float cpuTimeMs, float gpuTimeMs, bool gpuAvailable)
{
    ImGui::Begin("GPU Compute");

    if (!gpuAvailable)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "GPU compute not available");
        ImGui::Text("Using CPU fallback");
    }
    else
    {
        ImGui::Checkbox("Use GPU", &useGpu);

        ImGui::Separator();
        ImGui::Text("Generation Times:");
        ImGui::BulletText("CPU: %.2f ms", cpuTimeMs);
        ImGui::BulletText("GPU: %.2f ms", gpuTimeMs);

        if (cpuTimeMs > 0.0f && gpuTimeMs > 0.0f)
        {
            float speedup = cpuTimeMs / gpuTimeMs;
            ImGui::Text("Speedup: %.1fx", speedup);
        }
    }

    ImGui::End();
}

bool Gui::DrawEarthTerrainPanel(EarthTerrainSettings& settings, uint32_t& seed, int& subdivisions)
{
    bool needsRegeneration = false;

    ImGui::Begin("Earth Terrain");

    // Basic settings
    ImGui::Text("Basic Settings");
    ImGui::Separator();

    if (ImGui::SliderInt("Subdivisions", &subdivisions, 3, 7))
    {
        needsRegeneration = true;
    }

    int seedInt = static_cast<int>(seed);
    if (ImGui::InputInt("Seed", &seedInt))
    {
        seed = static_cast<uint32_t>(seedInt);
        needsRegeneration = true;
    }

    if (ImGui::SliderFloat("Height Scale", &settings.heightScale, 0.01f, 0.15f, "%.3f"))
    {
        needsRegeneration = true;
    }

    // Continental shape
    ImGui::Spacing();
    ImGui::Text("Continental Shape");
    ImGui::Separator();

    if (ImGui::SliderInt("Continent Octaves", &settings.continentOctaves, 2, 8))
    {
        needsRegeneration = true;
    }

    if (ImGui::SliderFloat("Continent Scale", &settings.continentScale, 0.3f, 3.0f))
    {
        needsRegeneration = true;
    }

    if (ImGui::SliderFloat("Continent Strength", &settings.continentStrength, 0.3f, 3.0f))
    {
        needsRegeneration = true;
    }

    if (ImGui::SliderFloat("Continent Persistence", &settings.continentPersistence, 0.2f, 0.8f))
    {
        needsRegeneration = true;
    }

    // Ocean settings
    ImGui::Spacing();
    ImGui::Text("Ocean Settings");
    ImGui::Separator();

    if (ImGui::SliderFloat("Ocean Depth Mult", &settings.oceanDepthMultiplier, 1.0f, 10.0f))
    {
        needsRegeneration = true;
    }

    if (ImGui::SliderFloat("Ocean Floor Depth", &settings.oceanFloorDepth, 0.5f, 3.0f))
    {
        needsRegeneration = true;
    }

    if (ImGui::SliderFloat("Ocean Floor Smooth", &settings.oceanFloorSmoothing, 0.1f, 2.0f))
    {
        needsRegeneration = true;
    }

    // Mountain ridges
    ImGui::Spacing();
    ImGui::Text("Mountain Ridges");
    ImGui::Separator();

    if (ImGui::SliderInt("Mountain Octaves", &settings.mountainOctaves, 3, 8))
    {
        needsRegeneration = true;
    }

    if (ImGui::SliderFloat("Mountain Scale", &settings.mountainScale, 0.5f, 4.0f))
    {
        needsRegeneration = true;
    }

    if (ImGui::SliderFloat("Mountain Strength", &settings.mountainStrength, 0.2f, 2.0f))
    {
        needsRegeneration = true;
    }

    if (ImGui::SliderFloat("Mountain Power", &settings.mountainPower, 1.0f, 5.0f))
    {
        needsRegeneration = true;
    }

    if (ImGui::SliderFloat("Mountain Gain", &settings.mountainGain, 0.5f, 2.0f))
    {
        needsRegeneration = true;
    }

    if (ImGui::SliderFloat("Mountain Blend", &settings.mountainBlend, 0.3f, 3.0f))
    {
        needsRegeneration = true;
    }

    // Mask settings
    ImGui::Spacing();
    ImGui::Text("Mountain Mask");
    ImGui::Separator();

    if (ImGui::SliderInt("Mask Octaves", &settings.maskOctaves, 2, 6))
    {
        needsRegeneration = true;
    }

    if (ImGui::SliderFloat("Mask Scale", &settings.maskScale, 0.2f, 2.0f))
    {
        needsRegeneration = true;
    }

    // Regenerate button
    ImGui::Spacing();
    ImGui::Separator();
    if (ImGui::Button("Regenerate Planet"))
    {
        needsRegeneration = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("Random Seed"))
    {
        seed = static_cast<uint32_t>(std::rand());
        needsRegeneration = true;
    }

    ImGui::End();

    return needsRegeneration;
}

bool Gui::DrawLodPanel(
    bool& useLodSystem,
    int& patchSubdivisions,
    float& planetRadius,
    int patchCount,
    int visiblePatchCount,
    int vertexCount)
{
    bool needsRegeneration = false;

    ImGui::Begin("LOD System");

    ImGui::Checkbox("Enable LOD System", &useLodSystem);

    if (useLodSystem)
    {
        ImGui::Separator();
        ImGui::Text("Settings");

        if (ImGui::SliderFloat("Planet Radius", &planetRadius, 1.0f, 50.0f))
        {
            needsRegeneration = true;
        }

        const char* subdivLabels[] = { "20 patches", "80 patches", "320 patches", "1280 patches" };
        if (ImGui::Combo("Patch Density", &patchSubdivisions, subdivLabels, 4))
        {
            needsRegeneration = true;
        }

        ImGui::Separator();
        ImGui::Text("Statistics");
        ImGui::BulletText("Total patches: %d", patchCount);
        ImGui::BulletText("Visible patches: %d", visiblePatchCount);
        ImGui::BulletText("Rendered vertices: %d", vertexCount);

        if (patchCount > 0)
        {
            float cullPercent = 100.0f * (1.0f - static_cast<float>(visiblePatchCount) / patchCount);
            ImGui::BulletText("Culled: %.1f%%", cullPercent);
        }

        ImGui::Separator();
        if (ImGui::Button("Regenerate LOD"))
        {
            needsRegeneration = true;
        }
    }

    ImGui::End();

    return needsRegeneration;
}

void Gui::DrawOceanPanel(effects::OceanSettings& settings, float& seaLevel)
{
    ImGui::Begin("Ocean");

    ImGui::Checkbox("Enable Ocean", &settings.enabled);

    if (settings.enabled)
    {
        ImGui::Separator();

        ImGui::SliderFloat("Sea Level", &seaLevel, 0.9f, 1.1f, "%.3f");

        ImGui::Text("Colors");
        ImGui::ColorEdit3("Deep Color", &settings.deepColor.x);
        ImGui::ColorEdit3("Shallow Color", &settings.shallowColor.x);

        ImGui::Separator();
        ImGui::Text("Reflection");
        ImGui::SliderFloat("Fresnel Power", &settings.fresnelPower, 1.0f, 5.0f);
        ImGui::SliderFloat("Specular Power", &settings.specularPower, 16.0f, 128.0f);
    }

    ImGui::End();
}

void Gui::DrawAtmospherePanel(effects::AtmosphereSettings& settings)
{
    ImGui::Begin("Atmosphere");

    ImGui::Checkbox("Enable Atmosphere", &settings.enabled);

    if (settings.enabled)
    {
        ImGui::Separator();

        ImGui::ColorEdit3("Color", &settings.color.x);
        ImGui::SliderFloat("Height", &settings.height, 0.01f, 0.2f, "%.3f");
        ImGui::SliderFloat("Density Falloff", &settings.densityFalloff, 1.0f, 10.0f);
        ImGui::SliderFloat("Intensity", &settings.intensity, 0.1f, 2.0f);
    }

    ImGui::End();
}

} // namespace planets::render
