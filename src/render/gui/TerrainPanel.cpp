#include "TerrainPanel.h"
#include "../settings/TerrainSettings.h"
#include "../BodyRuntime.h"
#include "GuiLayout.h"
#include <imgui.h>
#include <cstdlib>

namespace planets::render
{

bool TerrainPanel::Draw(BodyRuntime* activeBody,
                        GenerationConfig& config,
                        LodConfig& lod,
                        const TerrainStats& stats,
                        bool& visible,
                        bool& randomizeRequested)
{
    if (!visible)
        return false;

    bool regen = false;
    ImGui::Begin("Terrain", &visible);
    PushLabelColumn();

    if (activeBody && ImGui::CollapsingHeader("Generation", ImGuiTreeNodeFlags_DefaultOpen))
        regen |= DrawTerrainBlocks(*activeBody, config.seed, randomizeRequested);

    if (ImGui::CollapsingHeader("LOD System"))
        regen |= DrawLodContent(lod, stats);

    ImGui::PopItemWidth();
    ImGui::End();
    return regen;
}

bool TerrainPanel::DrawTerrainBlocks(BodyRuntime& body, uint32_t& seed, bool& randomizeRequested)
{
    bool regen = false;
    auto& sh = body.Config().shape;
    auto& tec = body.Config().tectonics;
    auto& ofl = body.Config().oceanFloor;
    auto& hd = body.Config().heightDetail;
    auto& mw = body.Config().macroWorld;

    // Regenerate only when a slider's edit is committed (mouse released), not on
    // every drag frame — otherwise every pixel of a drag would rebuild the planet.
    auto edited = [&]()
    {
        if (ImGui::IsItemDeactivatedAfterEdit())
            regen = true;
    };

    ImGui::Text("Basic Settings");
    ImGui::Separator();
    int seedInt = static_cast<int>(seed);
    if (ImGui::InputInt("Seed", &seedInt))
        seed = static_cast<uint32_t>(seedInt);
    edited();
    ImGui::SliderFloat("Height Scale", &sh.heightScale, 0.02f, 0.20f, "%.3f");
    edited();
    ImGui::SliderFloat("Global Frequency", &sh.globalFrequency, 0.6f, 2.0f, "%.2f");
    ImGui::SetItemTooltip("Scales every terrain noise frequency: higher = smaller features");
    edited();

    ImGui::Spacing();
    ImGui::Text("Tectonic Plates");
    ImGui::Separator();
    if (ImGui::Checkbox("Use Tectonics", &tec.enabled))
        regen = true;
    if (tec.enabled)
    {
        ImGui::SliderInt("Num Plates", &tec.numPlates, 6, 20);
        edited();
        ImGui::SliderFloat("Continental Fraction", &tec.continentalFraction, 0.2f, 0.7f);
        edited();
        ImGui::SliderFloat("Boundary Width", &tec.boundaryWidth, 0.05f, 0.4f);
        edited();
        ImGui::SliderFloat("Convergent Mountains", &tec.convergentMountainScale, 0.1f, 1.5f);
        edited();
        ImGui::SliderFloat("Divergent Rift Depth", &tec.divergentRiftDepth, 0.0f, 0.8f);
        edited();
        ImGui::SliderFloat("Coastline Noise", &tec.coastlineNoise, 0.0f, 1.0f);
        ImGui::SetItemTooltip("Only used when the continent mask is off");
        edited();
        ImGui::SliderFloat("Plate Interior Noise", &tec.plateElevationNoise, 0.0f, 0.5f);
        edited();
    }

    ImGui::Spacing();
    ImGui::Text("Continental Mask");
    ImGui::Separator();
    ImGui::SliderInt("Continent Count", &sh.continentCount, 1, 12);
    edited();
    ImGui::SliderFloat("Size Variance", &sh.continentSizeVariance, 0.0f, 1.0f, "%.2f");
    edited();
    ImGui::SliderFloat("Clustering", &sh.continentClustering, 0.0f, 1.0f, "%.2f");
    edited();
    ImGui::SliderInt("Mask Resolution", &sh.continentMaskResolution, 32, 256);
    edited();

    ImGui::Spacing();
    ImGui::Text("Continental Shape");
    ImGui::Separator();
    ImGui::SliderInt("Continent Octaves", &sh.continentNoise.octaves, 4, 7);
    edited();
    ImGui::SliderFloat("Continent Scale", &sh.continentNoise.scale, 0.5f, 1.5f);
    edited();
    ImGui::SliderFloat("Continent Strength", &sh.continentNoise.strength, 0.8f, 2.5f);
    edited();
    ImGui::SliderFloat("Continent Persistence", &sh.continentNoise.persistence, 0.35f, 0.65f);
    edited();
    ImGui::SliderFloat("Continent Lacunarity", &sh.continentNoise.lacunarity, 1.8f, 2.5f);
    edited();
    ImGui::SliderFloat("Base Level", &sh.continentBaseLevel, -0.6f, 0.2f);
    edited();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Negative = more ocean, positive = more land");

    ImGui::Spacing();
    ImGui::Text("Ocean Settings");
    ImGui::Separator();
    ImGui::SliderFloat("Ocean Depth Mult", &sh.oceanDepthMultiplier, 2.0f, 7.0f);
    edited();
    ImGui::SliderFloat("Ocean Floor Depth", &sh.oceanFloorDepth, 0.8f, 2.0f);
    edited();
    ImGui::SliderFloat("Ocean Floor Smooth", &sh.oceanFloorSmoothing, 0.3f, 1.2f);
    edited();

    ImGui::Spacing();
    ImGui::Text("Ocean Floor Topology");
    ImGui::Separator();
    if (ImGui::Checkbox("Use Ocean Floor", &ofl.enabled))
        regen = true;
    if (ofl.enabled)
    {
        ImGui::SliderFloat("Shelf Width", &ofl.shelfWidth, 0.05f, 0.4f);
        edited();
        ImGui::SliderInt("Ridge Octaves", &ofl.oceanRidgeOctaves, 2, 6);
        edited();
        ImGui::SliderFloat("Ridge Scale", &ofl.oceanRidgeScale, 0.3f, 2.0f);
        edited();
        ImGui::SliderFloat("Ridge Strength", &ofl.oceanRidgeStrength, 0.05f, 0.8f);
        edited();
        ImGui::SliderInt("Trench Octaves", &ofl.trenchOctaves, 2, 5);
        edited();
        ImGui::SliderFloat("Trench Scale", &ofl.trenchScale, 0.5f, 3.0f);
        edited();
        ImGui::SliderFloat("Trench Depth", &ofl.trenchDepth, 0.1f, 0.8f);
        edited();
        ImGui::SliderInt("Abyssal Octaves", &ofl.abyssalOctaves, 2, 6);
        edited();
        ImGui::SliderFloat("Abyssal Scale", &ofl.abyssalScale, 0.5f, 4.0f);
        ImGui::SetItemTooltip("Abyssal plain: the flat deep-ocean floor. Scale of its bumps");
        edited();
        ImGui::SliderFloat("Abyssal Strength", &ofl.abyssalStrength, 0.02f, 0.3f);
        ImGui::SetItemTooltip("Height of the bumps on the deep-ocean floor");
        edited();
    }

    ImGui::Spacing();
    ImGui::Text("Mountain Ridges");
    ImGui::Separator();
    ImGui::SliderInt("Mountain Octaves", &sh.mountainNoise.octaves, 4, 6);
    edited();
    ImGui::SliderFloat("Mountain Scale", &sh.mountainNoise.scale, 0.8f, 2.5f);
    edited();
    ImGui::SliderFloat("Mountain Strength", &sh.mountainNoise.strength, 0.4f, 1.3f);
    edited();
    ImGui::SliderFloat("Mountain Power", &sh.mountainPower, 1.5f, 3.0f);
    edited();
    ImGui::SliderFloat("Mountain Gain", &sh.mountainGain, 0.6f, 1.2f);
    edited();
    ImGui::SliderFloat("Mountain Lacunarity", &sh.mountainNoise.lacunarity, 2.0f, 5.0f);
    edited();
    ImGui::SliderFloat("Mountain Smoothing", &sh.mountainSmoothing, 0.5f, 2.0f);
    edited();
    ImGui::SliderFloat("Mountain Blend", &sh.mountainBlend, 0.6f, 2.0f);
    edited();

    ImGui::Spacing();
    ImGui::Text("Mountain Mask");
    ImGui::Separator();
    ImGui::SliderInt("Mask Octaves", &sh.maskNoise.octaves, 2, 4);
    edited();
    ImGui::SliderFloat("Mask Scale", &sh.maskNoise.scale, 0.5f, 1.5f);
    edited();
    ImGui::SliderFloat("Mask Lacunarity", &sh.maskNoise.lacunarity, 1.4f, 2.2f);
    edited();

    ImGui::Spacing();
    ImGui::Text("Height-Dependent Detail");
    ImGui::Separator();
    ImGui::SliderFloat("Detail Low Threshold", &hd.detailLowThreshold, -0.5f, 0.1f);
    edited();
    ImGui::SliderFloat("Detail High Threshold", &hd.detailHighThreshold, 0.0f, 0.8f);
    edited();
    ImGui::SliderFloat("Strength Low", &hd.perturbStrengthLow, 0.0f, 0.003f, "%.4f");
    edited();
    ImGui::SliderFloat("Strength High", &hd.perturbStrengthHigh, 0.001f, 0.01f, "%.4f");
    edited();
    ImGui::SliderInt("Octaves Low", &hd.detailOctavesLow, 1, 4);
    edited();
    ImGui::SliderInt("Octaves High", &hd.detailOctavesHigh, 2, 8);
    edited();
    ImGui::SliderFloat("Detail Persistence", &hd.detailPersistence, 0.2f, 0.7f);
    edited();
    ImGui::SliderFloat("Detail Lacunarity", &hd.detailLacunarity, 1.5f, 3.5f);
    edited();
    ImGui::SliderFloat("Perturb Scale", &hd.perturbScale, 10.0f, 30.0f);
    ImGui::SetItemTooltip("Frequency of the small height wobble added on top of the terrain");
    edited();

    // Once-per-body bake (plates, erosion, rivers, moisture). Any change here re-bakes.
    ImGui::Spacing();
    ImGui::Text("Macro World");
    ImGui::Separator();
    if (ImGui::Checkbox("Enable Macro World", &mw.enabled))
        regen = true;
    if (mw.enabled)
    {
        if (!tec.enabled)
            ImGui::TextDisabled("Needs tectonics (continent mask).");
        ImGui::SliderInt("Macro Resolution", &mw.resolution, 128, 1024);
        edited();
        ImGui::SliderFloat("Convergent Threshold", &mw.convergentThreshold, -0.5f, 0.9f, "%.2f");
        ImGui::SetItemTooltip("How hard plates must push together to raise mountains (higher = fewer ranges)");
        edited();
        ImGui::SliderInt("Macro Erosion Iterations", &mw.erosionIterations, 0, 1000);
        edited();
        ImGui::SliderFloat("Rain Rate", &mw.rainRate, 0.001f, 0.05f, "%.3f");
        edited();
        ImGui::SliderFloat("Macro Erosion Rate", &mw.erosionRate, 0.0f, 0.5f, "%.3f");
        edited();
        ImGui::SliderFloat("Macro Deposition Rate", &mw.depositionRate, 0.0f, 0.5f, "%.3f");
        edited();
        ImGui::SliderInt("Moisture Iterations", &mw.moistureIterations, 0, 512);
        edited();
        ImGui::SliderFloat("Wind Strength", &mw.windStrength, 0.5f, 8.0f, "%.1f");
        ImGui::SetItemTooltip("How far moisture travels inland per step");
        edited();
        ImGui::SliderFloat("Land Drying", &mw.landDrying, 0.0f, 0.1f, "%.3f");
        ImGui::SetItemTooltip("Moisture lost per step over land: higher = drier interiors");
        edited();
        ImGui::SliderFloat("Orographic Strength", &mw.orographicStrength, 0.0f, 10.0f, "%.2f");
        ImGui::SetItemTooltip("Rain lost when air climbs a slope: higher = stronger rain shadows");
        edited();
    }

    ImGui::Spacing();
    ImGui::Separator();
    if (ImGui::Button("Regenerate Planet"))
        regen = true;
    ImGui::SameLine();
    if (ImGui::Button("Random Seed"))
    {
        seed = static_cast<uint32_t>(std::rand());
        regen = true;
    }
    if (ImGui::Button("Randomize All"))
        randomizeRequested = true;

    return regen;
}

bool TerrainPanel::DrawLodContent(LodConfig& lod, const TerrainStats& stats)
{
    bool regen = false;
    auto edited = [&]()
    {
        if (ImGui::IsItemDeactivatedAfterEdit())
            regen = true;
    };

    ImGui::Text("Settings");
    ImGui::SliderFloat("Planet Radius", &lod.planetRadius, 1.0f, 5000.0f, "%.0f", ImGuiSliderFlags_Logarithmic);
    edited();

    const char* subdivLabels[] = {"20 patches", "80 patches", "320 patches", "1280 patches"};
    if (ImGui::Combo("Patch Density", &lod.patchSubdivisions, subdivLabels, 4))
        regen = true;

    ImGui::Separator();
    ImGui::Text("Quadtree");
    ImGui::SliderInt("Mesh Resolution", &lod.meshResolution, 8, 64);
    edited();
    ImGui::SliderInt("Max Depth", &lod.maxDepth, 2, 10);
    edited();
    ImGui::SliderFloat("Split Threshold", &lod.splitThreshold, 0.5f, 12.0f);
    ImGui::SetItemTooltip("Higher = patches split sooner, so denser terrain up close");
    edited();
    ImGui::SliderFloat("Hysteresis", &lod.hysteresis, 1.0f, 2.0f);
    ImGui::SetItemTooltip("Gap between the split and merge distances, so patches don't flicker");
    edited();
    ImGui::SliderInt("Max Active Patches", &lod.maxActivePatches, 50, 4000);
    ImGui::SetItemTooltip("Patch budget; splitting stops once it is reached");
    edited();
    ImGui::SliderFloat("Skirt Fraction", &lod.skirtFraction, 0.0f, 0.1f);
    ImGui::SetItemTooltip("Depth of the patch-edge skirts that hide cracks between LOD levels");
    edited();

    ImGui::Separator();
    ImGui::Text("Statistics");
    ImGui::BulletText("Total patches: %d", stats.patchCount);
    ImGui::BulletText("Visible patches: %d", stats.visiblePatchCount);
    ImGui::BulletText("GPU: terrain %.2f ms, ocean %.2f ms, effects %.2f ms",
                      stats.gpuTerrainMs, stats.gpuOceanMs, stats.gpuEffectsMs);
    if (stats.patchCount > 0)
    {
        float cullPercent = 100.0f * (1.0f - static_cast<float>(stats.visiblePatchCount) / stats.patchCount);
        ImGui::BulletText("Culled: %.1f%%", cullPercent);
    }

    ImGui::Separator();
    if (ImGui::Button("Regenerate LOD"))
        regen = true;
    return regen;
}

} // namespace planets::render
