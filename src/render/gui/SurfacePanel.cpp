#include "SurfacePanel.h"
#include "../settings/OceanSettings.h"
#include "../BodyRuntime.h"
#include "GuiLayout.h"
#include <imgui.h>

namespace planets::render
{

bool SurfacePanel::Draw(BodyRuntime* activeBody, effects::OceanSettings& ocean, float& seaLevel, bool& visible)
{
    if (!visible)
        return false;

    bool regen = false;
    ImGui::Begin("Surface", &visible);
    PushLabelColumn();

    if (activeBody)
        regen |= DrawShadingBlock(*activeBody);

    if (activeBody && activeBody->HasSolidSurface())
    {
        if (ImGui::CollapsingHeader("Ocean"))
            DrawOceanContent(ocean, seaLevel);
    }

    ImGui::PopItemWidth();
    ImGui::End();
    return regen;
}

bool SurfacePanel::DrawShadingBlock(BodyRuntime& body)
{
    bool regen = false;
    auto& sh = body.Config().shading;

    ImGui::Text("Body: %s", body.GetTypeName().c_str());
    ImGui::Separator();

    // Only the biome fields the active fragment shaders read. The other ShadingBlock biome and
    // colour fields feed the legacy planet_earth.frag, which no shipped body uses.
    if (ImGui::CollapsingHeader("Biomes", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("Snow Line", &sh.snowLine, 0.0f, 1.0f);
        ImGui::SetItemTooltip("Normalized height where snow starts (colder climates lower it)");
        ImGui::SliderFloat("AO Strength", &sh.aoStrength, 0.0f, 1.0f);
        ImGui::SetItemTooltip("Darkening of creases and valleys");
    }

    if (ImGui::CollapsingHeader("Shading Noise"))
    {
        ImGui::SliderFloat("Detail Scale", &sh.detailNoiseScale, 0.5f, 8.0f);
        if (ImGui::IsItemDeactivatedAfterEdit())
            regen = true;
        ImGui::SliderFloat("Small Scale", &sh.smallNoiseScale, 5.0f, 40.0f);
        if (ImGui::IsItemDeactivatedAfterEdit())
            regen = true;
        ImGui::SliderInt("Small Octaves", &sh.smallNoiseOctaves, 2, 8);
        if (ImGui::IsItemDeactivatedAfterEdit())
            regen = true;
        ImGui::SliderFloat("Warp Strength", &sh.warpStrength, 0.0f, 1.0f);
        if (ImGui::IsItemDeactivatedAfterEdit())
            regen = true;
    }

    if (ImGui::CollapsingHeader("Climate Model"))
    {
        if (ImGui::Checkbox("Use Climate Model", &sh.useClimateModel))
            regen = true;
        if (sh.useClimateModel)
        {
            ImGui::SliderFloat("Large Scale", &sh.largeNoiseScale, 0.2f, 0.7f);
            if (ImGui::IsItemDeactivatedAfterEdit())
                regen = true;
            ImGui::SliderInt("Large Octaves", &sh.largeNoiseOctaves, 2, 4);
            if (ImGui::IsItemDeactivatedAfterEdit())
                regen = true;
            ImGui::SliderFloat("Lapse Rate", &sh.temperatureLapseRate, 0.5f, 4.0f, "%.1f");
            ImGui::SetItemTooltip("How much colder it gets with altitude");
            if (ImGui::IsItemDeactivatedAfterEdit())
                regen = true;
            ImGui::SliderFloat("Temp Exponent", &sh.temperatureExponent, 0.3f, 1.5f, "%.2f");
            ImGui::SetItemTooltip("Shape of the equator-to-pole temperature falloff");
            if (ImGui::IsItemDeactivatedAfterEdit())
                regen = true;
            ImGui::SliderFloat("Moisture Scale", &sh.moistureNoiseScale, 0.5f, 3.0f, "%.1f");
            if (ImGui::IsItemDeactivatedAfterEdit())
                regen = true;
            ImGui::SliderFloat("Moisture Noise", &sh.moistureNoiseStrength, 0.0f, 0.4f, "%.2f");
            if (ImGui::IsItemDeactivatedAfterEdit())
                regen = true;
            ImGui::SliderFloat("Hadley Intensity", &sh.hadleyIntensity, 0.0f, 2.0f, "%.1f");
            ImGui::SetItemTooltip("Strength of the latitude rain bands: wet tropics, dry subtropics");
            if (ImGui::IsItemDeactivatedAfterEdit())
                regen = true;
            ImGui::SliderFloat("Continentality", &sh.continentalityStrength, 0.0f, 0.6f, "%.2f");
            ImGui::SetItemTooltip("How much wetter coasts are than high interiors");
            if (ImGui::IsItemDeactivatedAfterEdit())
                regen = true;
        }
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
        ImGui::SetItemTooltip("Water surface radius (1 = planet radius). Terrain shading, the ocean and the "
                              "atmosphere all use this one value");

        ImGui::Separator();
        ImGui::Text("Water");
        ImGui::ColorEdit3("Shallow Color", &settings.shallowColor.x);
        ImGui::SetItemTooltip("Water colour over the shallows");
        ImGui::ColorEdit3("Deep Color", &settings.deepColor.x);
        ImGui::SetItemTooltip("Water colour over deep ocean");
        ImGui::SliderFloat("Clarity", &settings.clarity, 0.02f, 1.0f, "%.2f");
        ImGui::SetItemTooltip("How deep you can see into the water before it turns opaque");

        ImGui::Separator();
        ImGui::Text("Waves");
        ImGui::SliderFloat("Wave Strength", &settings.waveStrength, 0.0f, 0.5f, "%.2f");
        ImGui::SetItemTooltip("Steepness of the waves: rougher water, wider sun glitter");
        ImGui::SliderFloat("Wave Scale", &settings.waveScale, 0.25f, 4.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        ImGui::SetItemTooltip("Wave frequency relative to planet size: higher = smaller waves");
        ImGui::SliderFloat("Wave Speed", &settings.waveSpeed, 0.1f, 2.0f, "%.1f");
        ImGui::SetItemTooltip("How fast the waves move");
    }
}

} // namespace planets::render
