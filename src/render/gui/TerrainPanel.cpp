#include "TerrainPanel.h"
#include "../settings/TerrainSettings.h"
#include <imgui.h>
#include <string>
#include <cstdlib>

namespace planets::render
{

bool TerrainPanel::Draw(GenerationConfig& config,
                        EarthTerrainSettings& terrain,
                        LodConfig& lod,
                        const TerrainStats& stats,
                        bool& visible,
                        bool& randomizeRequested)
{
    if (!visible)
        return false;

    bool regen = false;
    ImGui::Begin("Terrain", &visible);

    if (ImGui::CollapsingHeader("GPU Compute"))
        DrawGpuContent(config, stats);

    if (ImGui::CollapsingHeader("Generation", ImGuiTreeNodeFlags_DefaultOpen))
        regen |= DrawEarthTerrainContent(terrain, config.seed, config.subdivisions, randomizeRequested);

    if (ImGui::CollapsingHeader("LOD System"))
        regen |= DrawLodContent(lod, stats);

    ImGui::End();
    return regen;
}

void TerrainPanel::DrawGpuContent(GenerationConfig& config, const TerrainStats& stats)
{
    (void)config;
    ImGui::Text("Generation Times:");
    ImGui::BulletText("GPU: %.2f ms", stats.gpuTimeMs);
}

bool TerrainPanel::DrawEarthTerrainContent(EarthTerrainSettings& settings,
                                           uint32_t& seed,
                                           int& subdivisions,
                                           bool& randomizeRequested)
{
    bool needsRegeneration = false;

    ImGui::Text("Basic Settings");
    ImGui::Separator();

    ImGui::SliderInt("Subdivisions", &subdivisions, 3, 7);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    int seedInt = static_cast<int>(seed);
    if (ImGui::InputInt("Seed", &seedInt))
    {
        seed = static_cast<uint32_t>(seedInt);
    }
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderFloat("Height Scale", &settings.heightScale, 0.02f, 0.08f, "%.3f");
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(
            "Terrain height as fraction of planet radius.\n0.01 = subtle hills, 0.10 = dramatic mountains");

    ImGui::SliderFloat("Global Frequency", &settings.globalFrequency, 0.6f, 2.0f, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Multiplier for all noise frequencies.\nHigher = more detail = planet feels larger.\n1.0 = "
                          "default, 2.0 = twice as detailed");

    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                       "Tip: Lower height scale + higher frequency = larger-feeling planet");

    ImGui::Spacing();
    ImGui::Text("Tectonic Plates");
    ImGui::Separator();

    if (ImGui::Checkbox("Use Tectonics", &settings.useTectonics))
        needsRegeneration = true;

    if (settings.useTectonics)
    {
        ImGui::SliderInt("Num Plates", &settings.numPlates, 6, 20);
        if (ImGui::IsItemDeactivatedAfterEdit())
            needsRegeneration = true;

        ImGui::SliderFloat("Continental Fraction", &settings.continentalFraction, 0.2f, 0.7f);
        if (ImGui::IsItemDeactivatedAfterEdit())
            needsRegeneration = true;

        ImGui::SliderFloat("Boundary Width", &settings.boundaryWidth, 0.05f, 0.4f);
        if (ImGui::IsItemDeactivatedAfterEdit())
            needsRegeneration = true;

        ImGui::SliderFloat("Convergent Mountains", &settings.convergentMountainScale, 0.1f, 1.5f);
        if (ImGui::IsItemDeactivatedAfterEdit())
            needsRegeneration = true;

        ImGui::SliderFloat("Divergent Rift Depth", &settings.divergentRiftDepth, 0.0f, 0.8f);
        if (ImGui::IsItemDeactivatedAfterEdit())
            needsRegeneration = true;

        ImGui::SliderFloat("Coastline Noise", &settings.coastlineNoise, 0.0f, 1.0f);
        if (ImGui::IsItemDeactivatedAfterEdit())
            needsRegeneration = true;

        ImGui::SliderFloat("Plate Interior Noise", &settings.plateElevationNoise, 0.0f, 0.5f);
        if (ImGui::IsItemDeactivatedAfterEdit())
            needsRegeneration = true;
    }

    ImGui::Spacing();
    ImGui::Text("Continental Shape");
    ImGui::Separator();

    ImGui::SliderInt("Continent Octaves", &settings.continentOctaves, 4, 7);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderFloat("Continent Scale", &settings.continentScale, 0.5f, 1.5f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderFloat("Continent Strength", &settings.continentStrength, 0.8f, 2.5f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderFloat("Continent Persistence", &settings.continentPersistence, 0.35f, 0.65f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderFloat("Continent Lacunarity", &settings.continentLacunarity, 1.8f, 2.5f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderFloat("Base Level", &settings.continentBaseLevel, -0.6f, -0.1f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Negative = more ocean, Positive = more land");

    ImGui::Spacing();
    ImGui::Text("Ocean Settings");
    ImGui::Separator();

    ImGui::SliderFloat("Ocean Depth Mult", &settings.oceanDepthMultiplier, 2.0f, 7.0f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderFloat("Ocean Floor Depth", &settings.oceanFloorDepth, 0.8f, 2.0f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderFloat("Ocean Floor Smooth", &settings.oceanFloorSmoothing, 0.3f, 1.2f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::Spacing();
    ImGui::Text("Ocean Floor Topology");
    ImGui::Separator();

    if (ImGui::Checkbox("Use Ocean Floor", &settings.useOceanFloor))
        needsRegeneration = true;

    if (settings.useOceanFloor)
    {
        ImGui::SliderFloat("Shelf Width", &settings.shelfWidth, 0.05f, 0.4f);
        if (ImGui::IsItemDeactivatedAfterEdit())
            needsRegeneration = true;

        ImGui::SliderInt("Ridge Octaves", &settings.oceanRidgeOctaves, 2, 6);
        if (ImGui::IsItemDeactivatedAfterEdit())
            needsRegeneration = true;

        ImGui::SliderFloat("Ridge Scale", &settings.oceanRidgeScale, 0.3f, 2.0f);
        if (ImGui::IsItemDeactivatedAfterEdit())
            needsRegeneration = true;

        ImGui::SliderFloat("Ridge Strength", &settings.oceanRidgeStrength, 0.05f, 0.8f);
        if (ImGui::IsItemDeactivatedAfterEdit())
            needsRegeneration = true;

        ImGui::SliderInt("Trench Octaves", &settings.trenchOctaves, 2, 5);
        if (ImGui::IsItemDeactivatedAfterEdit())
            needsRegeneration = true;

        ImGui::SliderFloat("Trench Scale", &settings.trenchScale, 0.5f, 3.0f);
        if (ImGui::IsItemDeactivatedAfterEdit())
            needsRegeneration = true;

        ImGui::SliderFloat("Trench Depth", &settings.trenchDepth, 0.1f, 0.8f);
        if (ImGui::IsItemDeactivatedAfterEdit())
            needsRegeneration = true;

        ImGui::SliderInt("Abyssal Octaves", &settings.abyssalOctaves, 2, 6);
        if (ImGui::IsItemDeactivatedAfterEdit())
            needsRegeneration = true;

        ImGui::SliderFloat("Abyssal Scale", &settings.abyssalScale, 0.5f, 4.0f);
        if (ImGui::IsItemDeactivatedAfterEdit())
            needsRegeneration = true;

        ImGui::SliderFloat("Abyssal Strength", &settings.abyssalStrength, 0.02f, 0.3f);
        if (ImGui::IsItemDeactivatedAfterEdit())
            needsRegeneration = true;
    }

    ImGui::Spacing();
    ImGui::Text("Mountain Ridges");
    ImGui::Separator();

    ImGui::SliderInt("Mountain Octaves", &settings.mountainOctaves, 4, 6);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderFloat("Mountain Scale", &settings.mountainScale, 0.8f, 2.5f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderFloat("Mountain Strength", &settings.mountainStrength, 0.4f, 1.3f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderFloat("Mountain Power", &settings.mountainPower, 1.5f, 3.0f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderFloat("Mountain Gain", &settings.mountainGain, 0.6f, 1.2f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderFloat("Mountain Lacunarity", &settings.mountainLacunarity, 2.0f, 5.0f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderFloat("Mountain Smoothing", &settings.mountainSmoothing, 0.5f, 2.0f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderFloat("Mountain Blend", &settings.mountainBlend, 0.6f, 2.0f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::Spacing();
    ImGui::Text("Mountain Mask");
    ImGui::Separator();

    ImGui::SliderInt("Mask Octaves", &settings.maskOctaves, 2, 4);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderFloat("Mask Scale", &settings.maskScale, 0.5f, 1.5f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderFloat("Mask Lacunarity", &settings.maskLacunarity, 1.4f, 2.2f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::Spacing();
    ImGui::Text("Height-Dependent Detail");
    ImGui::Separator();

    ImGui::SliderFloat("Detail Low Threshold", &settings.detailLowThreshold, -0.5f, 0.1f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Height below this = minimal detail (smooth plains)");

    ImGui::SliderFloat("Detail High Threshold", &settings.detailHighThreshold, 0.0f, 0.8f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Height above this = maximum detail (rough mountains)");

    ImGui::SliderFloat("Strength Low", &settings.perturbStrengthLow, 0.0f, 0.003f, "%.4f");
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Perturbation strength for flat terrain");

    ImGui::SliderFloat("Strength High", &settings.perturbStrengthHigh, 0.001f, 0.01f, "%.4f");
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Perturbation strength for elevated terrain");

    ImGui::SliderInt("Octaves Low", &settings.detailOctavesLow, 1, 4);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderInt("Octaves High", &settings.detailOctavesHigh, 2, 8);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderFloat("Detail Persistence", &settings.detailPersistence, 0.2f, 0.7f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderFloat("Detail Lacunarity", &settings.detailLacunarity, 1.5f, 3.5f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderFloat("Perturb Scale", &settings.perturbScale, 10.0f, 30.0f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Base frequency of perturbation noise");

    ImGui::Spacing();
    ImGui::Text("Erosion");
    ImGui::Separator();

    if (ImGui::Checkbox("Enable Erosion", &settings.enableErosion))
        needsRegeneration = true;

    if (settings.enableErosion)
    {
        ImGui::SliderInt("Erosion Iterations", &settings.erosionIterations, 1, 15);
        if (ImGui::IsItemDeactivatedAfterEdit())
            needsRegeneration = true;
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("More iterations = smoother, more eroded terrain");

        ImGui::SliderFloat("Thermal Rate", &settings.thermalErosionRate, 0.001f, 0.1f, "%.3f");
        if (ImGui::IsItemDeactivatedAfterEdit())
            needsRegeneration = true;
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Rate of slope-based material redistribution");

        ImGui::SliderFloat("Thermal Threshold", &settings.thermalThreshold, 0.001f, 0.05f, "%.3f");
        if (ImGui::IsItemDeactivatedAfterEdit())
            needsRegeneration = true;
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Minimum slope angle before thermal erosion activates");

        ImGui::SliderFloat("Hydraulic Rate", &settings.hydraulicErosionRate, 0.001f, 0.05f, "%.3f");
        if (ImGui::IsItemDeactivatedAfterEdit())
            needsRegeneration = true;

        ImGui::SliderFloat("Deposition Rate", &settings.depositionRate, 0.001f, 0.02f, "%.3f");
        if (ImGui::IsItemDeactivatedAfterEdit())
            needsRegeneration = true;

        ImGui::SliderFloat("Evaporation Rate", &settings.evaporationRate, 0.01f, 0.5f, "%.2f");
        if (ImGui::IsItemDeactivatedAfterEdit())
            needsRegeneration = true;
    }

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

    ImGui::SameLine();
    if (ImGui::Button("Randomize All"))
    {
        randomizeRequested = true;
    }

    return needsRegeneration;
}

bool TerrainPanel::DrawLodContent(LodConfig& lod, const TerrainStats& stats)
{
    bool needsRegeneration = false;

    ImGui::Text("Settings");

    ImGui::SliderFloat("Planet Radius", &lod.planetRadius, 1.0f, 5000.0f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    const char* subdivLabels[] = {"20 patches", "80 patches", "320 patches", "1280 patches"};
    if (ImGui::Combo("Patch Density", &lod.patchSubdivisions, subdivLabels, 4))
    {
        needsRegeneration = true;
    }

    // Quadtree parameters
    ImGui::Separator();
    ImGui::Text("Quadtree");

    ImGui::SliderInt("Mesh Resolution", &lod.meshResolution, 8, 64);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderInt("Max Depth", &lod.maxDepth, 2, 10);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderFloat("Split Threshold", &lod.splitThreshold, 0.5f, 5.0f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderFloat("Hysteresis", &lod.hysteresis, 1.0f, 2.0f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderInt("Max Active Patches", &lod.maxActivePatches, 50, 800);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

    ImGui::SliderFloat("Skirt Fraction", &lod.skirtFraction, 0.0f, 0.1f);
    if (ImGui::IsItemDeactivatedAfterEdit())
        needsRegeneration = true;

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

    return needsRegeneration;
}

} // namespace planets::render
