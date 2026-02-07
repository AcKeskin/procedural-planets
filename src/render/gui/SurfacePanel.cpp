#include "SurfacePanel.h"
#include "../settings/SurfaceSettings.h"
#include "../settings/TerrainSettings.h"
#include "../settings/OceanSettings.h"
#include <imgui.h>

namespace planets::render {

void SurfacePanel::Draw(BiomeSettings& biomes, EarthColors& colors,
                        EarthShadingSettings& shading,
                        effects::OceanSettings& ocean, float& seaLevel, bool& visible)
{
    if (!visible) return;

    ImGui::Begin("Surface", &visible);

    if (ImGui::CollapsingHeader("Biomes", ImGuiTreeNodeFlags_DefaultOpen))
        DrawBiomeContent(biomes);

    if (ImGui::CollapsingHeader("Colors"))
        DrawEarthColorsContent(colors);

    if (ImGui::CollapsingHeader("Shading Noise"))
        DrawShadingContent(shading);

    if (ImGui::CollapsingHeader("Ocean"))
        DrawOceanContent(ocean, seaLevel);

    ImGui::End();
}

void SurfacePanel::DrawBiomeContent(BiomeSettings& settings)
{
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
}

void SurfacePanel::DrawEarthColorsContent(EarthColors& colors)
{
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
}

void SurfacePanel::DrawShadingContent(EarthShadingSettings& settings)
{
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
}

void SurfacePanel::DrawOceanContent(effects::OceanSettings& settings, float& seaLevel)
{
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
}

} // namespace planets::render
