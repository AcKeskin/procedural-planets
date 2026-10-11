#include "TerrainTab.h"
#include "Widgets.h"
#include "../BodyRuntime.h"
#include <imgui.h>

namespace planets::render
{

bool TerrainTab::Draw(BodyRuntime& body)
{
    using namespace ui;
    bool regen = false;
    auto& cfg = body.Config();
    auto& sh = cfg.shape;
    auto& tec = cfg.tectonics;
    auto& ofl = cfg.oceanFloor;
    auto& mw = cfg.macroWorld;
    auto& ts = cfg.terrainStyle;

    if (ImGui::CollapsingHeader("Shape", ImGuiTreeNodeFlags_DefaultOpen))
    {
        regen |= SliderRegen("Radius",
                             cfg.metadata.radius,
                             1.0f,
                             5000.0f,
                             "%.0f",
                             "Planet radius in scene units. Changes scale and LOD, not shape. 1 to 5000.",
                             ImGuiSliderFlags_Logarithmic);
        regen |= SliderRegen("Relief",
                             sh.heightScale,
                             0.005f,
                             0.2f,
                             "%.3f",
                             "Mountain height and sea depth, as a fraction of radius. 0.005 to 0.2.");
        regen |= SliderRegen(
            "Frequency", sh.globalFrequency, 0.6f, 2.0f, "%.2f", "Higher = more, smaller landforms. 0.6 to 2.");
        regen |= SliderRegen("Land Amount",
                             sh.continentBaseLevel,
                             -0.6f,
                             0.2f,
                             "%.3f",
                             "Below 0 = more ocean, above 0 = more land. Water height is Ocean > Sea Level.");
    }

    if (ImGui::CollapsingHeader("Continents"))
    {
        regen |= SliderRegen("Count", sh.continentCount, 1, 12, "Number of continents. 1 to 12.");
        regen |= SliderRegen("Size Variance",
                             sh.continentSizeVariance,
                             0.0f,
                             1.0f,
                             "%.2f",
                             "0 = continents of equal size, 1 = a few big and many small.");
        regen |= SliderRegen("Clustering",
                             sh.continentClustering,
                             0.0f,
                             1.0f,
                             "%.2f",
                             "0 = spread over the globe, 1 = gathered into a supercontinent.");
        regen |= SliderRegen("Octaves", sh.continentNoise.octaves, 4, 7, "Layers of coastline detail. 4 to 7.");
        regen |= SliderRegen("Scale",
                             sh.continentNoise.scale,
                             0.5f,
                             1.5f,
                             "%.2f",
                             "Size of the coastline shapes: higher = smaller bays and capes.");
        regen |= SliderRegen("Strength",
                             sh.continentNoise.strength,
                             0.8f,
                             2.5f,
                             "%.2f",
                             "How strongly the coastline noise bends the continents.");
        regen |= SliderRegen("Persistence",
                             sh.continentNoise.persistence,
                             0.35f,
                             0.65f,
                             "%.2f",
                             "How much each finer detail layer adds. Higher = rougher coasts.");
        regen |= SliderRegen(
            "Lacunarity", sh.continentNoise.lacunarity, 1.8f, 2.5f, "%.2f", "Size step between detail layers.");
        if (ImGui::TreeNode("Advanced##continents"))
        {
            regen |= SliderRegen("Mask Resolution",
                                 sh.continentMaskResolution,
                                 32,
                                 256,
                                 "Detail of the baked continent map, in cells. Higher = slower bake.");
            ImGui::TreePop();
        }
    }

    if (ImGui::CollapsingHeader("Mountains"))
    {
        regen |= SliderRegen("Octaves##mountain", sh.mountainNoise.octaves, 4, 6, "Layers of ridge detail. 4 to 6.");
        regen |= SliderRegen("Scale##mountain",
                             sh.mountainNoise.scale,
                             0.8f,
                             2.5f,
                             "%.2f",
                             "Size of the ranges: higher = shorter, more ranges.");
        regen |= SliderRegen("Strength##mountain",
                             sh.mountainNoise.strength,
                             0.4f,
                             1.3f,
                             "%.2f",
                             "How tall the ranges stand over the land.");
        regen |= SliderRegen(
            "Power", sh.mountainPower, 1.5f, 3.0f, "%.2f", "Sharpness of the peaks: higher = steeper, narrower peaks.");
        regen |= SliderRegen("Gain", sh.mountainGain, 0.6f, 1.2f, "%.2f", "How much each finer ridge layer adds.");
        regen |= SliderRegen(
            "Lacunarity##mountain", sh.mountainNoise.lacunarity, 2.0f, 5.0f, "%.2f", "Size step between ridge layers.");
        regen |= SliderRegen("Smoothing",
                             sh.mountainSmoothing,
                             0.5f,
                             2.0f,
                             "%.2f",
                             "Rounds off the ridge crests: higher = softer mountains.");
        regen |= SliderRegen(
            "Blend", sh.mountainBlend, 0.6f, 2.0f, "%.2f", "How far the mountains spread into the lowlands.");
        if (ImGui::TreeNode("Placement"))
        {
            regen |= SliderRegen(
                "Mask Octaves", sh.maskNoise.octaves, 2, 4, "Detail of the map that decides where ranges go.");
            regen |= SliderRegen("Mask Scale",
                                 sh.maskNoise.scale,
                                 0.5f,
                                 1.5f,
                                 "%.2f",
                                 "Size of the mountain regions: higher = more, smaller regions.");
            regen |= SliderRegen(
                "Mask Lacunarity", sh.maskNoise.lacunarity, 1.4f, 2.2f, "%.2f", "Size step between placement layers.");
            ImGui::TreePop();
        }
    }

    if (ImGui::CollapsingHeader("Tectonic Plates"))
    {
        regen |= Check("Tectonic Plates##on",
                       tec.enabled,
                       "Builds land from plates: mountains where they meet, rifts where they split.");
        ImGui::BeginDisabled(!tec.enabled);
        regen |= SliderRegen("Plates", tec.numPlates, 6, 20, "Number of plates. 6 to 20.");
        regen |= SliderRegen(
            "Land Plates", tec.continentalFraction, 0.2f, 0.7f, "%.2f", "Share of plates that carry continents.");
        regen |= SliderRegen(
            "Boundary Width", tec.boundaryWidth, 0.05f, 0.4f, "%.2f", "Width of the zone where plates meet.");
        regen |= SliderRegen("Collision Mountains",
                             tec.convergentMountainScale,
                             0.1f,
                             1.5f,
                             "%.2f",
                             "Height of the ranges where plates push together.");
        regen |= SliderRegen(
            "Rift Depth", tec.divergentRiftDepth, 0.0f, 0.8f, "%.2f", "Depth of the valleys where plates pull apart.");
        regen |=
            SliderRegen("Coastline Noise", tec.coastlineNoise, 0.0f, 1.0f, "%.2f", "Roughness of plate coastlines.");
        regen |= SliderRegen(
            "Plate Interior Noise", tec.plateElevationNoise, 0.0f, 0.5f, "%.2f", "Height variation inside each plate.");
        ImGui::EndDisabled();
    }

    if (ImGui::CollapsingHeader("Sea Floor Shape"))
    {
        regen |= SliderRegen("Ocean Depth",
                             sh.oceanDepthMultiplier,
                             2.0f,
                             7.0f,
                             "%.2f",
                             "How deep the oceans are compared with mountain height.");
        regen |= SliderRegen(
            "Floor Depth", sh.oceanFloorDepth, 0.8f, 2.0f, "%.2f", "Depth of the abyssal floor below the shelf.");
        regen |= SliderRegen("Floor Smoothing",
                             sh.oceanFloorSmoothing,
                             0.3f,
                             1.2f,
                             "%.2f",
                             "How gently the shelf falls off into deep water.");
        regen |= Check("Floor Detail", ofl.enabled, "Ridges, trenches and abyssal plains on the sea floor.");
        ImGui::BeginDisabled(!ofl.enabled);
        regen |= SliderRegen(
            "Shelf Width", ofl.shelfWidth, 0.05f, 0.4f, "%.2f", "Width of the shallow shelf around the continents.");
        if (ImGui::TreeNode("Ridges"))
        {
            regen |= SliderRegen("Octaves##ridge", ofl.oceanRidgeOctaves, 2, 6, "Layers of ridge detail.");
            regen |= SliderRegen("Scale##ridge",
                                 ofl.oceanRidgeScale,
                                 0.3f,
                                 2.0f,
                                 "%.2f",
                                 "Size of the mid-ocean ridges: higher = more, shorter ridges.");
            regen |= SliderRegen(
                "Strength##ridge", ofl.oceanRidgeStrength, 0.05f, 0.8f, "%.2f", "Height of the mid-ocean ridges.");
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Trenches"))
        {
            regen |= SliderRegen("Octaves##trench", ofl.trenchOctaves, 2, 5, "Layers of trench detail.");
            regen |= SliderRegen("Scale##trench",
                                 ofl.trenchScale,
                                 0.5f,
                                 3.0f,
                                 "%.2f",
                                 "Size of the trenches: higher = more, shorter trenches.");
            regen |= SliderRegen("Depth##trench", ofl.trenchDepth, 0.1f, 0.8f, "%.2f", "Depth of the trenches.");
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Abyssal Plains"))
        {
            regen |= SliderRegen("Octaves##abyssal", ofl.abyssalOctaves, 2, 6, "Layers of bumps on the deep floor.");
            regen |= SliderRegen(
                "Scale##abyssal", ofl.abyssalScale, 0.5f, 4.0f, "%.2f", "Size of the bumps on the deep floor.");
            regen |= SliderRegen("Strength##abyssal",
                                 ofl.abyssalStrength,
                                 0.02f,
                                 0.3f,
                                 "%.2f",
                                 "Height of the bumps on the deep floor.");
            ImGui::TreePop();
        }
        ImGui::EndDisabled();
    }

    if (ImGui::CollapsingHeader("Terrain Style"))
    {
        regen |= SliderRegen("Dunes", ts.dunes, 0.0f, 3.0f, "%.2f", "Wind-aligned sand ridges on the land. 0 = off.");
        regen |= SliderRegen("Dune Frequency", ts.duneScale, 0.1f, 3.0f, "%.2f", "Higher = smaller, denser dunes.");
        regen |= SliderRegen(
            "Craters", ts.craters, 0.0f, 2.0f, "%.2f", "Impact craters with raised rims, everywhere. 0 = off.");
        regen |= SliderRegen(
            "Crater Frequency", ts.craterScale, 0.1f, 3.0f, "%.2f", "Higher = smaller, more numerous craters.");
        regen |= SliderRegen(
            "Terraces", ts.terraces, 0.0f, 1.0f, "%.2f", "Stepped land: flat mesa tops and cliffs. 0 = off.");
        regen |=
            SliderRegen("Terrace Steps", ts.terraceSteps, 1.0f, 30.0f, "%.0f", "Number of steps over the land height.");
        regen |=
            SliderRegen("Canyons", ts.canyons, 0.0f, 2.0f, "%.2f", "Narrow carved channels and sinkholes. 0 = off.");
        regen |= SliderRegen("Spires", ts.spires, 0.0f, 2.0f, "%.2f", "Tall pointed rock or crystal spikes. 0 = off.");
    }

    if (ImGui::CollapsingHeader("Erosion Sim (slow)"))
    {
        regen |= Check(
            "Erosion Sim", mw.enabled, "Rain, erosion and moisture at planet scale. Slow: each change reruns it.");
        if (mw.enabled && !tec.enabled)
            ImGui::TextDisabled("Needs Tectonic Plates.");
        ImGui::BeginDisabled(!mw.enabled);
        regen |= SliderRegen("Resolution", mw.resolution, 128, 1024, "Cells per cube face. Higher = finer, slower.");
        regen |= SliderRegen("Collision Threshold",
                             mw.convergentThreshold,
                             -0.5f,
                             0.9f,
                             "%.2f",
                             "How hard plates must push to raise mountains. Higher = fewer ranges.");
        regen |=
            SliderRegen("Iterations", mw.erosionIterations, 0, 1000, "Erosion steps. More = deeper valleys, slower.");
        regen |= SliderRegen("Rain", mw.rainRate, 0.001f, 0.05f, "%.3f", "Water added per step.");
        regen |= SliderRegen("Erosion Rate", mw.erosionRate, 0.0f, 0.5f, "%.3f", "How fast water cuts the ground.");
        regen |=
            SliderRegen("Deposition Rate", mw.depositionRate, 0.0f, 0.5f, "%.3f", "How fast carried sediment settles.");
        regen |= SliderRegen(
            "Moisture Iterations", mw.moistureIterations, 0, 512, "Steps of wind carrying moisture inland.");
        regen |= SliderRegen("Wind", mw.windStrength, 0.5f, 8.0f, "%.1f", "How far moisture travels inland per step.");
        regen |= SliderRegen("Land Drying",
                             mw.landDrying,
                             0.0f,
                             0.1f,
                             "%.3f",
                             "Moisture lost per step over land. Higher = drier interiors.");
        regen |= SliderRegen("Mountain Rain",
                             mw.orographicStrength,
                             0.0f,
                             10.0f,
                             "%.2f",
                             "Rain lost when air climbs a slope. Higher = stronger rain shadows.");
        ImGui::EndDisabled();
    }

    return regen;
}

} // namespace planets::render
