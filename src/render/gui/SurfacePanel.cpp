#include "SurfacePanel.h"
#include "../settings/OceanSettings.h"
#include "../settings/SurfaceSettings.h"
#include "../BodyRuntime.h"
#include "GuiLayout.h"
#include <imgui.h>

namespace planets::render
{

bool SurfacePanel::Draw(BodyRuntime* activeBody, SurfaceSettings& surface, effects::OceanSettings& ocean, float& seaLevel,
                        bool& visible)
{
    if (!visible)
        return false;

    bool regen = false;
    ImGui::Begin("Surface", &visible);
    PushLabelColumn();

    if (activeBody)
    {
        regen |= DrawShadingBlock(*activeBody);
        regen |= DrawPlanetType(*activeBody, surface);
    }

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

bool SurfacePanel::DrawPlanetType(BodyRuntime& body, SurfaceSettings& surface)
{
    bool regen = false;
    if (ImGui::CollapsingHeader("Planet Type"))
    {
        ImGui::Text("Materials");
        ImGui::ColorEdit3("Rock Tint", &surface.rockTint.x);
        ImGui::SetItemTooltip("Multiplies the rock on slopes (1 = Earth)");
        ImGui::ColorEdit3("Sand Tint", &surface.sandTint.x);
        ImGui::SetItemTooltip("Multiplies beaches, deserts and dunes (1 = Earth)");
        ImGui::ColorEdit3("Snow Tint", &surface.snowTint.x);
        ImGui::SetItemTooltip("Multiplies snow and ice caps (1 = Earth)");
        ImGui::SliderFloat("Vegetation", &surface.vegetation, 0.0f, 1.0f, "%.2f");
        ImGui::SetItemTooltip("Grass texture on the flats: 0 = bare soil (dead worlds), 1 = Earth");
        ImGui::SliderFloat("Sand Cover", &surface.sandCover, 0.0f, 1.0f, "%.2f");
        ImGui::SetItemTooltip("Sand on every flat: 1 = dune sea");

        ImGui::Separator();
        ImGui::Text("Climate");
        ImGui::SliderFloat("Polar Caps", &surface.polarCaps, 0.0f, 0.8f, "%.2f");
        ImGui::SetItemTooltip("Ice caps from each pole, also on hot worlds (0 = Earth's climate only)");
        ImGui::SliderFloat("Tidally Locked", &surface.substellar, 0.0f, 1.0f, "%.2f");
        ImGui::SetItemTooltip("1 = eye world: hot desert under the sun, a living ring, frozen night side");
        ImGui::SliderFloat("City Coverage", &surface.cityCoverage, 0.0f, 1.0f, "%.2f");
        ImGui::SetItemTooltip("City lights on all land, not only habitable land (Scene > Night Lights sets "
                              "the brightness)");

        ImGui::Separator();
        ImGui::Text("Terrain Style (regenerates)");
        auto& ts = body.Config().terrainStyle;
        auto style = [&regen](const char* label, float& v, float max, const char* tip)
        {
            ImGui::SliderFloat(label, &v, 0.0f, max, "%.2f");
            ImGui::SetItemTooltip("%s", tip);
            if (ImGui::IsItemDeactivatedAfterEdit())
                regen = true;
        };
        style("Dunes", ts.dunes, 3.0f, "Wind-aligned sand ridges on the land");
        style("Dune Scale", ts.duneScale, 3.0f, "Higher = smaller, denser dunes");
        style("Craters", ts.craters, 2.0f, "Impact craters with raised rims, everywhere");
        style("Crater Scale", ts.craterScale, 3.0f, "Higher = smaller, more numerous craters");
        style("Terraces", ts.terraces, 1.0f, "Stepped land: flat mesa tops and cliffs");
        ImGui::SliderFloat("Terrace Steps", &ts.terraceSteps, 1.0f, 30.0f, "%.0f");
        ImGui::SetItemTooltip("Number of steps over the land height");
        if (ImGui::IsItemDeactivatedAfterEdit())
            regen = true;
        style("Canyons", ts.canyons, 2.0f, "Narrow carved channels and sinkholes");
        style("Spires", ts.spires, 2.0f, "Tall pointed rock or crystal spikes");
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

        ImGui::Separator();
        ImGui::Text("Fluid");
        ImGui::SliderFloat("Glow", &settings.emissive, 0.0f, 5.0f, "%.2f");
        ImGui::SetItemTooltip("The fluid glows in its own colour: lava ~3, acid a little");
        ImGui::SliderFloat("Crust", &settings.crust, 0.0f, 1.0f, "%.2f");
        ImGui::SetItemTooltip("Share of the surface under dark floating crust, glowing cracks between");
        ImGui::ColorEdit3("Crust Color", &settings.crustColor.x);
        ImGui::SliderFloat("Frozen", &settings.frozen, 0.0f, 1.0f, "%.2f");
        ImGui::SetItemTooltip("1 = ice: no waves, opaque, cracked");
        ImGui::SliderFloat("Reflectivity", &settings.reflectivity, 0.0f, 2.0f, "%.2f");
        ImGui::SetItemTooltip("Scales the sky reflection and sun glint (lava barely reflects)");
        ImGui::ColorEdit3("Foam Color", &settings.foamColor.x);
        ImGui::SetItemTooltip("Surf along the shore");
    }
}

} // namespace planets::render
