#include "SurfacePanel.h"
#include "../settings/OceanSettings.h"
#include "../BodyRuntime.h"
#include <imgui.h>

namespace planets::render
{

bool SurfacePanel::Draw(BodyRuntime* activeBody,
                        effects::OceanSettings& ocean,
                        float& seaLevel,
                        bool& visible)
{
    if (!visible)
        return false;

    bool regen = false;
    ImGui::Begin("Surface", &visible);

    if (activeBody)
        regen |= DrawShadingBlock(*activeBody);

    if (activeBody && activeBody->HasSolidSurface())
    {
        if (ImGui::CollapsingHeader("Ocean"))
            DrawOceanContent(ocean, seaLevel);
    }

    ImGui::End();
    return regen;
}

bool SurfacePanel::DrawShadingBlock(BodyRuntime& body)
{
    bool regen = false;
    auto& sh = body.Config().shading;

    ImGui::Text("Body: %s", body.GetTypeName().c_str());
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Biomes", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Enable Biomes", &sh.biomesEnabled);
        if (sh.biomesEnabled)
        {
            ImGui::SliderFloat("Steepness Threshold", &sh.steepnessThreshold, 0.0f, 1.0f);
            ImGui::SliderFloat("Flat→Steep Blend", &sh.flatToSteepBlend, 0.0f, 0.5f);
            ImGui::SliderFloat("Snow Latitude", &sh.snowLatitude, 0.0f, 1.0f);
            ImGui::SliderFloat("Snow Blend", &sh.snowBlend, 0.0f, 0.5f);
            ImGui::SliderFloat("Snow Line", &sh.snowLine, 0.0f, 1.0f);
            ImGui::SliderFloat("Shore Height", &sh.shoreHeight, 0.0f, 0.3f);
            ImGui::SliderFloat("AO Strength", &sh.aoStrength, 0.0f, 1.0f);
        }
    }

    if (ImGui::CollapsingHeader("Shading Noise"))
    {
        ImGui::SliderFloat("Detail Scale", &sh.detailNoiseScale, 0.5f, 8.0f);
        if (ImGui::IsItemDeactivatedAfterEdit()) regen = true;
        ImGui::SliderFloat("Small Scale", &sh.smallNoiseScale, 5.0f, 40.0f);
        if (ImGui::IsItemDeactivatedAfterEdit()) regen = true;
        ImGui::SliderInt("Small Octaves", &sh.smallNoiseOctaves, 2, 8);
        if (ImGui::IsItemDeactivatedAfterEdit()) regen = true;
        ImGui::SliderFloat("Warp Strength", &sh.warpStrength, 0.0f, 1.0f);
        if (ImGui::IsItemDeactivatedAfterEdit()) regen = true;
    }

    if (ImGui::CollapsingHeader("Climate Model"))
    {
        if (ImGui::Checkbox("Use Climate Model", &sh.useClimateModel)) regen = true;
        if (sh.useClimateModel)
        {
            ImGui::SliderFloat("Large Scale", &sh.largeNoiseScale, 0.2f, 0.7f);
            if (ImGui::IsItemDeactivatedAfterEdit()) regen = true;
            ImGui::SliderInt("Large Octaves", &sh.largeNoiseOctaves, 2, 4);
            if (ImGui::IsItemDeactivatedAfterEdit()) regen = true;
            ImGui::SliderFloat("Lapse Rate", &sh.temperatureLapseRate, 0.5f, 4.0f, "%.1f");
            if (ImGui::IsItemDeactivatedAfterEdit()) regen = true;
            ImGui::SliderFloat("Temp Exponent", &sh.temperatureExponent, 0.3f, 1.5f, "%.2f");
            if (ImGui::IsItemDeactivatedAfterEdit()) regen = true;
            ImGui::SliderFloat("Moisture Scale", &sh.moistureNoiseScale, 0.5f, 3.0f, "%.1f");
            if (ImGui::IsItemDeactivatedAfterEdit()) regen = true;
            ImGui::SliderFloat("Moisture Noise", &sh.moistureNoiseStrength, 0.0f, 0.4f, "%.2f");
            if (ImGui::IsItemDeactivatedAfterEdit()) regen = true;
            ImGui::SliderFloat("Hadley Intensity", &sh.hadleyIntensity, 0.0f, 2.0f, "%.1f");
            if (ImGui::IsItemDeactivatedAfterEdit()) regen = true;
            ImGui::SliderFloat("Continentality", &sh.continentalityStrength, 0.0f, 0.6f, "%.2f");
            if (ImGui::IsItemDeactivatedAfterEdit()) regen = true;
        }
    }

    if (ImGui::CollapsingHeader("Colours"))
    {
        ImGui::SliderFloat("Flat Col Blend", &sh.flatColBlend, 0.8f, 2.5f);
        ImGui::SliderFloat("Flat Col Noise", &sh.flatColBlendNoise, 0.1f, 0.5f);
        ImGui::Separator();
        ImGui::ColorEdit3("Shore Low",  &sh.colorShoreLow.x);
        ImGui::ColorEdit3("Shore High", &sh.colorShoreHigh.x);
        ImGui::ColorEdit3("Flat A Low",  &sh.colorFlatLowA.x);
        ImGui::ColorEdit3("Flat A High", &sh.colorFlatHighA.x);
        ImGui::ColorEdit3("Flat B Low",  &sh.colorFlatLowB.x);
        ImGui::ColorEdit3("Flat B High", &sh.colorFlatHighB.x);
        ImGui::ColorEdit3("Steep Low",  &sh.colorSteepLow.x);
        ImGui::ColorEdit3("Steep High", &sh.colorSteepHigh.x);
        ImGui::ColorEdit3("Snow", &sh.colorSnow.x);
    }

    return regen;
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

        ImGui::Separator();
        ImGui::Text("Waves");
        ImGui::SliderFloat("Wave Strength", &settings.waveStrength, 0.0f, 0.5f, "%.2f");
        ImGui::SliderFloat("Wave Scale", &settings.waveScale, 5.0f, 50.0f);
        ImGui::SliderFloat("Wave Speed", &settings.waveSpeed, 0.1f, 2.0f, "%.1f");
    }
}

} // namespace planets::render
