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

        ImGui::SliderFloat("Shore Zone", &settings.shoreHeight, 0.04f, 0.15f, "%.2f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Beach height above sea level");

        ImGui::SliderFloat("Snow Line", &settings.snowLine, 0.70f, 0.95f, "%.2f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Height where snow starts on peaks");

        ImGui::Separator();
        ImGui::Text("Cliff Rock");

        ImGui::SliderFloat("Rock Threshold", &settings.steepnessThreshold, 0.15f, 0.45f, "%.2f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Steepness for rock texture\nLower = more rock");
        ImGui::SliderFloat("Rock Blend", &settings.flatToSteepBlend, 0.05f, 0.20f, "%.2f");

        ImGui::Separator();
        ImGui::Text("Polar Snow");

        ImGui::SliderFloat("Snow Latitude", &settings.snowLatitude, 0.65f, 0.85f, "%.2f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Latitude for polar snow");
        ImGui::SliderFloat("Snow Blend", &settings.snowBlend, 0.05f, 0.15f, "%.2f");
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

    ImGui::SliderFloat("Large Scale", &settings.largeNoiseScale, 0.2f, 0.7f);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Climate zone scale");

    ImGui::SliderInt("Large Octaves", &settings.largeNoiseOctaves, 2, 4);

    ImGui::SliderFloat("Detail Scale", &settings.detailNoiseScale, 1.0f, 4.0f);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Terrain texture variation");

    ImGui::SliderFloat("Small Scale", &settings.smallNoiseScale, 8.0f, 25.0f);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("High-frequency surface detail");

    ImGui::SliderInt("Small Octaves", &settings.smallNoiseOctaves, 3, 6);

    ImGui::SliderFloat("Warp Strength", &settings.warpStrength, 0.1f, 0.6f);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Biome boundary irregularity");

    ImGui::Separator();
    ImGui::Text("Color Blending");

    ImGui::SliderFloat("Flat Col Blend", &settings.flatColBlend, 0.8f, 2.5f);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Height threshold for low/high gradient");

    ImGui::SliderFloat("Flat Col Noise", &settings.flatColBlendNoise, 0.1f, 0.5f);
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
        ImGui::Text("Depth");
        ImGui::SliderFloat("Depth Multiplier", &settings.depthMultiplier, 1.0f, 30.0f);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Controls shallow-to-deep color transition");
        ImGui::SliderFloat("Alpha Multiplier", &settings.alphaMultiplier, 10.0f, 100.0f);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Controls transparency based on depth");

        ImGui::Separator();
        ImGui::Text("Reflection");
        ImGui::SliderFloat("Fresnel Power", &settings.fresnelPower, 1.5f, 3.5f);
        ImGui::SliderFloat("Smoothness", &settings.smoothness, 0.5f, 0.99f, "%.2f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Higher = sharper, more reflective highlight");

        ImGui::Separator();
        ImGui::Text("Waves");
        ImGui::SliderFloat("Wave Strength", &settings.waveStrength, 0.0f, 0.5f, "%.2f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Normal perturbation intensity");
        ImGui::SliderFloat("Wave Scale", &settings.waveScale, 5.0f, 50.0f);
        ImGui::SliderFloat("Wave Speed", &settings.waveSpeed, 0.1f, 2.0f, "%.1f");
    }
}

} // namespace planets::render
