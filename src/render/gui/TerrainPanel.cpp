#include "TerrainPanel.h"
#include "../settings/TerrainSettings.h"
#include "../../core/generation/Planet.h"
#include "../../core/generation/NoiseLayer.h"
#include <imgui.h>
#include <string>
#include <cstdlib>

namespace planets::render {

bool TerrainPanel::Draw(GenerationConfig& config, EarthTerrainSettings& terrain,
                        LodConfig& lod, const TerrainStats& stats,
                        planets::core::Planet& planet, bool& visible)
{
    if (!visible) return false;

    bool regen = false;
    ImGui::Begin("Terrain", &visible);

    if (ImGui::CollapsingHeader("GPU Compute"))
        DrawGpuContent(config, stats);

    if (ImGui::CollapsingHeader("Generation", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (config.useGpu)
            regen |= DrawEarthTerrainContent(terrain, config.seed, config.subdivisions);
        else
            regen |= DrawPlanetContent(planet);
    }

    if (ImGui::CollapsingHeader("LOD System"))
        regen |= DrawLodContent(lod, stats);

    ImGui::End();
    return regen;
}

void TerrainPanel::DrawGpuContent(GenerationConfig& config, const TerrainStats& stats)
{
    if (!stats.gpuAvailable)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "GPU compute not available");
        ImGui::Text("Using CPU fallback");
    }
    else
    {
        ImGui::Checkbox("Use GPU", &config.useGpu);

        ImGui::Separator();
        ImGui::Text("Generation Times:");
        ImGui::BulletText("CPU: %.2f ms", stats.cpuTimeMs);
        ImGui::BulletText("GPU: %.2f ms", stats.gpuTimeMs);

        if (stats.cpuTimeMs > 0.0f && stats.gpuTimeMs > 0.0f)
        {
            float speedup = stats.cpuTimeMs / stats.gpuTimeMs;
            ImGui::Text("Speedup: %.1fx", speedup);
        }
    }
}

bool TerrainPanel::DrawEarthTerrainContent(EarthTerrainSettings& settings, uint32_t& seed, int& subdivisions)
{
    bool needsRegeneration = false;

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

    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Tip: Lower height scale + higher frequency = larger-feeling planet");

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

    ImGui::Spacing();
    ImGui::Text("Ocean Settings");
    ImGui::Separator();

    ImGui::SliderFloat("Ocean Depth Mult", &settings.oceanDepthMultiplier, 1.0f, 10.0f);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

    ImGui::SliderFloat("Ocean Floor Depth", &settings.oceanFloorDepth, 0.5f, 3.0f);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

    ImGui::SliderFloat("Ocean Floor Smooth", &settings.oceanFloorSmoothing, 0.1f, 2.0f);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

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

    ImGui::Spacing();
    ImGui::Text("Mountain Mask");
    ImGui::Separator();

    ImGui::SliderInt("Mask Octaves", &settings.maskOctaves, 2, 6);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

    ImGui::SliderFloat("Mask Scale", &settings.maskScale, 0.2f, 2.0f);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

    ImGui::Spacing();
    ImGui::Text("Surface Detail");
    ImGui::Separator();

    ImGui::SliderFloat("Perturb Strength", &settings.perturbStrength, 0.0f, 0.01f, "%.4f");
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("High-frequency micro-detail roughness");

    ImGui::SliderFloat("Perturb Scale", &settings.perturbScale, 5.0f, 50.0f);
    if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Frequency of perturbation noise");

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

    return needsRegeneration;
}

bool TerrainPanel::DrawPlanetContent(planets::core::Planet& planet)
{
    bool needsRegeneration = false;

    auto& settings = planet.GetSettings();

    ImGui::Text("Planet Settings");
    ImGui::Separator();

    if (ImGui::SliderFloat("Radius", &settings.radius, 0.5f, 5.0f))
    {
        needsRegeneration = true;
    }

    if (ImGui::SliderFloat("Sea Level", &settings.seaLevel, 0.0f, 1.0f))
    {
        // Sea level doesn't require mesh regeneration
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

    return needsRegeneration;
}

bool TerrainPanel::DrawLodContent(LodConfig& lod, const TerrainStats& stats)
{
    bool needsRegeneration = false;

    ImGui::Checkbox("Enable LOD System", &lod.enabled);

    if (lod.enabled)
    {
        ImGui::Separator();
        ImGui::Text("Settings");

        ImGui::SliderFloat("Planet Radius", &lod.planetRadius, 1.0f, 50.0f);
        if (ImGui::IsItemDeactivatedAfterEdit()) needsRegeneration = true;

        const char* subdivLabels[] = { "20 patches", "80 patches", "320 patches", "1280 patches" };
        if (ImGui::Combo("Patch Density", &lod.patchSubdivisions, subdivLabels, 4))
        {
            needsRegeneration = true;
        }

        ImGui::Separator();
        ImGui::Text("Statistics");
        ImGui::BulletText("Total patches: %d", stats.patchCount);
        ImGui::BulletText("Visible patches: %d", stats.visiblePatchCount);
        ImGui::BulletText("Rendered vertices: %d", stats.vertexCount);

        if (stats.patchCount > 0)
        {
            float cullPercent = 100.0f * (1.0f - static_cast<float>(stats.visiblePatchCount) / stats.patchCount);
            ImGui::BulletText("Culled: %.1f%%", cullPercent);
        }

        ImGui::Separator();
        if (ImGui::Button("Regenerate LOD"))
        {
            needsRegeneration = true;
        }
    }

    return needsRegeneration;
}

} // namespace planets::render
