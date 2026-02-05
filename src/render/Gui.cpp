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

    ImGui::SliderInt("Subdivisions", &subdivisions, 3, 7);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

    int seedInt = static_cast<int>(seed);
    if (ImGui::InputInt("Seed", &seedInt))
    {
        seed = static_cast<uint32_t>(seedInt);
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

    ImGui::SliderFloat("Height Scale", &settings.heightScale, 0.01f, 0.15f, "%.3f");
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Terrain height as fraction of planet radius.\n0.01 = subtle hills, 0.10 = dramatic mountains");

    ImGui::SliderFloat("Global Frequency", &settings.globalFrequency, 0.5f, 4.0f, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Multiplier for all noise frequencies.\nHigher = more detail = planet feels larger.\n1.0 = default, 2.0 = twice as detailed");

    // Scale info
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Tip: Lower height scale + higher frequency = larger-feeling planet");

    // Continental shape
    ImGui::Spacing();
    ImGui::Text("Continental Shape");
    ImGui::Separator();

    ImGui::SliderInt("Continent Octaves", &settings.continentOctaves, 2, 8);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

    ImGui::SliderFloat("Continent Scale", &settings.continentScale, 0.3f, 3.0f);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

    ImGui::SliderFloat("Continent Strength", &settings.continentStrength, 0.3f, 3.0f);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

    ImGui::SliderFloat("Continent Persistence", &settings.continentPersistence, 0.2f, 0.8f);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

    ImGui::SliderFloat("Base Level", &settings.continentBaseLevel, -1.5f, 0.5f);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Negative = more ocean, Positive = more land");

    // Ocean settings
    ImGui::Spacing();
    ImGui::Text("Ocean Settings");
    ImGui::Separator();

    ImGui::SliderFloat("Ocean Depth Mult", &settings.oceanDepthMultiplier, 1.0f, 10.0f);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

    ImGui::SliderFloat("Ocean Floor Depth", &settings.oceanFloorDepth, 0.5f, 3.0f);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

    ImGui::SliderFloat("Ocean Floor Smooth", &settings.oceanFloorSmoothing, 0.1f, 2.0f);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

    // Mountain ridges
    ImGui::Spacing();
    ImGui::Text("Mountain Ridges");
    ImGui::Separator();

    ImGui::SliderInt("Mountain Octaves", &settings.mountainOctaves, 3, 8);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

    ImGui::SliderFloat("Mountain Scale", &settings.mountainScale, 0.5f, 4.0f);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

    ImGui::SliderFloat("Mountain Strength", &settings.mountainStrength, 0.2f, 2.0f);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

    ImGui::SliderFloat("Mountain Power", &settings.mountainPower, 1.0f, 5.0f);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

    ImGui::SliderFloat("Mountain Gain", &settings.mountainGain, 0.5f, 2.0f);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

    ImGui::SliderFloat("Mountain Blend", &settings.mountainBlend, 0.3f, 3.0f);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

    // Mask settings
    ImGui::Spacing();
    ImGui::Text("Mountain Mask");
    ImGui::Separator();

    ImGui::SliderInt("Mask Octaves", &settings.maskOctaves, 2, 6);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

    ImGui::SliderFloat("Mask Scale", &settings.maskScale, 0.2f, 2.0f);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

    // Surface Detail (Perturbation)
    ImGui::Spacing();
    ImGui::Text("Surface Detail");
    ImGui::Separator();

    ImGui::SliderFloat("Perturb Strength", &settings.perturbStrength, 0.0f, 0.01f, "%.4f");
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("High-frequency micro-detail roughness");

    ImGui::SliderFloat("Perturb Scale", &settings.perturbScale, 5.0f, 50.0f);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Frequency of perturbation noise");

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

        ImGui::SliderFloat("Planet Radius", &planetRadius, 1.0f, 50.0f);
        if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

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
        ImGui::Text("Geometry");
        ImGui::SliderFloat("Atmosphere Scale", &settings.atmosphereScale, 0.02f, 0.3f, "%.3f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Height as fraction of planet radius");

        ImGui::Separator();
        ImGui::Text("Rayleigh Scattering");

        // Wavelength controls (determines sky color)
        ImGui::SliderFloat("Red Wavelength", &settings.wavelengths.x, 550.0f, 780.0f, "%.0f nm");
        ImGui::SliderFloat("Green Wavelength", &settings.wavelengths.y, 450.0f, 600.0f, "%.0f nm");
        ImGui::SliderFloat("Blue Wavelength", &settings.wavelengths.z, 380.0f, 500.0f, "%.0f nm");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Shorter wavelengths scatter more (blue sky)");

        ImGui::SliderFloat("Scattering Strength", &settings.scatteringStrength, 1.0f, 30.0f);

        ImGui::Separator();
        ImGui::Text("Density");
        ImGui::SliderFloat("Density Falloff", &settings.densityFalloff, 1.0f, 20.0f);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("How quickly density decreases with altitude");

        ImGui::Separator();
        ImGui::Text("Quality");
        ImGui::SliderInt("In-Scatter Samples", &settings.numInScatteringPoints, 3, 20);
        ImGui::SliderInt("Optical Depth Samples", &settings.numOpticalDepthPoints, 3, 20);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("More samples = better quality, slower");

        ImGui::Separator();
        ImGui::SliderFloat("Intensity", &settings.intensity, 0.1f, 3.0f);

        ImGui::Separator();
        ImGui::Text("Dithering");
        ImGui::SliderFloat("Dither Strength", &settings.ditherStrength, 0.0f, 2.0f);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Blue noise dithering to reduce banding");
        ImGui::SliderFloat("Dither Scale", &settings.ditherScale, 1.0f, 10.0f);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Scale of the noise pattern");

        ImGui::Separator();
        if (ImGui::Button("Earth Preset"))
        {
            settings.wavelengths = glm::vec3(700.0f, 530.0f, 460.0f);
            settings.scatteringStrength = 21.23f;
            settings.densityFalloff = 4.3f;
            settings.atmosphereScale = 0.322f;
            settings.intensity = 1.0f;
            settings.ditherStrength = 0.8f;
            settings.ditherScale = 4.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Mars Preset"))
        {
            settings.wavelengths = glm::vec3(600.0f, 550.0f, 500.0f);
            settings.scatteringStrength = 3.0f;
            settings.densityFalloff = 4.0f;
            settings.atmosphereScale = 0.05f;
            settings.intensity = 0.6f;
            settings.ditherStrength = 0.8f;
            settings.ditherScale = 4.0f;
        }
    }

    ImGui::End();
}

void Gui::DrawBiomePanel(BiomeSettings& settings)
{
    ImGui::Begin("Biomes");

    ImGui::Checkbox("Enable Biomes", &settings.enabled);

    if (settings.enabled)
    {
        ImGui::Separator();
        ImGui::Text("Height Zones (0=sea, 1=max)");

        ImGui::SliderFloat("Shore Zone", &settings.shoreHeight, 0.02f, 0.25f, "%.2f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Beach height above sea level");

        ImGui::SliderFloat("Snow Line", &settings.snowLine, 0.5f, 0.98f, "%.2f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Height where snow starts on peaks");

        ImGui::Separator();
        ImGui::Text("Cliff Rock");

        ImGui::SliderFloat("Rock Threshold", &settings.steepnessThreshold, 0.1f, 0.5f, "%.2f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Steepness for rock texture\nLower = more rock");
        ImGui::SliderFloat("Rock Blend", &settings.flatToSteepBlend, 0.05f, 0.25f, "%.2f");

        ImGui::Separator();
        ImGui::Text("Polar Snow");

        ImGui::SliderFloat("Snow Latitude", &settings.snowLatitude, 0.5f, 0.95f, "%.2f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Latitude for polar snow");
        ImGui::SliderFloat("Snow Blend", &settings.snowBlend, 0.02f, 0.2f, "%.2f");

        ImGui::Separator();
        ImGui::Checkbox("Auto Height Range", &settings.autoHeightRange);

        if (!settings.autoHeightRange)
        {
            ImGui::SliderFloat("Height Min", &settings.heightRangeMin, 0.8f, 1.0f, "%.4f");
            ImGui::SliderFloat("Height Max", &settings.heightRangeMax, 1.0f, 1.2f, "%.4f");
        }
    }

    ImGui::End();
}

void Gui::DrawEarthColorsPanel(EarthColors& colors)
{
    ImGui::Begin("Earth Colors");

    ImGui::Text("Shore (Beach)");
    ImGui::ColorEdit3("Shore Low", &colors.shoreLow.x);
    ImGui::ColorEdit3("Shore High", &colors.shoreHigh.x);

    ImGui::Separator();
    ImGui::Text("Flat Terrain A (Plains)");
    ImGui::ColorEdit3("Flat A Low", &colors.flatLowA.x);
    ImGui::ColorEdit3("Flat A High", &colors.flatHighA.x);

    ImGui::Separator();
    ImGui::Text("Flat Terrain B (Forest)");
    ImGui::ColorEdit3("Flat B Low", &colors.flatLowB.x);
    ImGui::ColorEdit3("Flat B High", &colors.flatHighB.x);

    ImGui::Separator();
    ImGui::Text("Steep Terrain (Rock)");
    ImGui::ColorEdit3("Steep Low", &colors.steepLow.x);
    ImGui::ColorEdit3("Steep High", &colors.steepHigh.x);

    ImGui::Separator();
    ImGui::ColorEdit3("Snow", &colors.snow.x);

    ImGui::Separator();
    if (ImGui::Button("Reset to Earth Preset"))
    {
        colors = EarthColors();
    }

    ImGui::End();
}

void Gui::DrawShadingPanel(EarthShadingSettings& settings)
{
    ImGui::Begin("Shading Noise");

    ImGui::Text("Noise Scales");
    ImGui::Separator();

    ImGui::SliderFloat("Large Scale", &settings.largeNoiseScale, 0.1f, 1.0f);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Climate zone scale");

    ImGui::SliderInt("Large Octaves", &settings.largeNoiseOctaves, 1, 5);

    ImGui::SliderFloat("Detail Scale", &settings.detailNoiseScale, 0.5f, 5.0f);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Terrain texture variation");

    ImGui::SliderFloat("Small Scale", &settings.smallNoiseScale, 5.0f, 30.0f);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("High-frequency surface detail");

    ImGui::SliderInt("Small Octaves", &settings.smallNoiseOctaves, 3, 8);

    ImGui::SliderFloat("Warp Strength", &settings.warpStrength, 0.0f, 0.5f);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Biome boundary irregularity");

    ImGui::Separator();
    ImGui::Text("Color Blending");

    ImGui::SliderFloat("Flat Col Blend", &settings.flatColBlend, 0.5f, 3.0f);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Height threshold for low/high gradient");

    ImGui::SliderFloat("Flat Col Noise", &settings.flatColBlendNoise, 0.0f, 0.6f);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Noise influence on gradient blend");

    ImGui::End();
}

void Gui::DrawSpacePanel(glm::vec3& lightDir, float& sunSize, glm::vec3& sunColor, float& starDensity)
{
    ImGui::Begin("Space & Sun");

    ImGui::Text("Sun Direction");
    ImGui::Separator();

    // Convert to spherical for easier editing
    float lightLength = glm::length(lightDir);
    glm::vec3 normalizedDir = lightDir / lightLength;

    float azimuth = atan2(normalizedDir.x, normalizedDir.z);
    float elevation = asin(normalizedDir.y);

    bool changed = false;
    changed |= ImGui::SliderAngle("Azimuth", &azimuth, -180.0f, 180.0f);
    changed |= ImGui::SliderAngle("Elevation", &elevation, -90.0f, 90.0f);

    if (changed)
    {
        lightDir.x = cos(elevation) * sin(azimuth);
        lightDir.y = sin(elevation);
        lightDir.z = cos(elevation) * cos(azimuth);
        lightDir = glm::normalize(lightDir);
    }

    ImGui::Separator();
    ImGui::Text("Sun Appearance");

    ImGui::SliderFloat("Sun Size", &sunSize, 0.01f, 0.1f, "%.3f rad");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Angular size in radians");

    ImGui::ColorEdit3("Sun Color", &sunColor.x);

    ImGui::Separator();
    ImGui::Text("Stars");

    ImGui::SliderFloat("Star Density", &starDensity, 0.0f, 3.0f);

    ImGui::End();
}

void Gui::DrawLightingPanel(LightingSettings& settings)
{
    ImGui::Begin("Lighting");

    ImGui::Text("Sun Lighting");
    ImGui::Separator();

    ImGui::SliderFloat("Sun Intensity", &settings.sunIntensity, 0.1f, 5.0f, "%.2f");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Overall brightness of sunlight on terrain");

    ImGui::SliderFloat("Ambient Light", &settings.ambientLight, 0.0f, 0.5f, "%.3f");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Base illumination in shadows");

    ImGui::Separator();
    ImGui::Text("Specular");

    ImGui::SliderFloat("Specular Strength", &settings.specularStrength, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Specular Power", &settings.specularPower, 4.0f, 128.0f, "%.0f");

    ImGui::Separator();
    if (ImGui::Button("Reset"))
    {
        settings = LightingSettings();
    }

    ImGui::End();
}

} // namespace planets::render
