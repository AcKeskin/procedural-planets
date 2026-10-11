#include "SurfacePanel.h"
#include "../settings/OceanSettings.h"
#include "../settings/SurfaceSettings.h"
#include "../BodyRuntime.h"
#include "GuiLayout.h"
#include <imgui.h>

namespace planets::render
{

bool SurfacePanel::Draw(
    BodyRuntime* activeBody, SurfaceSettings& surface, effects::OceanSettings& ocean, float& seaLevel, bool& visible)
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
        DrawSurfaceRules(surface);
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
        ImGui::ColorEdit3("Rock Tint", &surface.rock.tint.x);
        ImGui::SetItemTooltip("Multiplies the rock on slopes (1 = Earth)");
        ImGui::ColorEdit3("Sand Tint", &surface.sand.tint.x);
        ImGui::SetItemTooltip("Multiplies beaches, deserts and dunes (1 = Earth)");
        ImGui::ColorEdit3("Snow Tint", &surface.snow.tint.x);
        ImGui::SetItemTooltip("Multiplies snow and ice caps (1 = Earth)");
        ImGui::SliderFloat("Vegetation", &surface.ground.vegetation, 0.0f, 1.0f, "%.2f");
        ImGui::SetItemTooltip("Grass texture on the flats: 0 = bare soil (dead worlds), 1 = Earth");
        ImGui::SliderFloat("Sand Cover", &surface.sand.cover, 0.0f, 1.0f, "%.2f");
        ImGui::SetItemTooltip("Sand on every flat: 1 = dune sea");

        ImGui::Separator();
        ImGui::Text("Climate");
        ImGui::SliderFloat("Polar Caps", &surface.climate.polarCaps, 0.0f, 0.8f, "%.2f");
        ImGui::SetItemTooltip("Ice caps from each pole, also on hot worlds (0 = Earth's climate only)");
        ImGui::SliderFloat("Tidally Locked", &surface.climate.substellar, 0.0f, 1.0f, "%.2f");
        ImGui::SetItemTooltip("1 = eye world: hot desert under the sun, a living ring, frozen night side");
        ImGui::SliderFloat("City Coverage", &surface.climate.cityCoverage, 0.0f, 1.0f, "%.2f");
        ImGui::SetItemTooltip("How much land is built up: low = separate cities, 1 = one planet-wide city "
                              "(Scene > Night Lights sets the brightness)");

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

void SurfacePanel::DrawSurfaceRules(SurfaceSettings& s)
{
    if (!ImGui::CollapsingHeader("Surface Rules"))
        return;
    auto color = [](const char* label, glm::vec3& c, const char* tip)
    {
        ImGui::ColorEdit3(label, &c.x);
        ImGui::SetItemTooltip("%s", tip);
    };
    auto value = [](const char* label, float& v, float lo, float hi, const char* tip)
    {
        ImGui::SliderFloat(label, &v, lo, hi, "%.3f");
        ImGui::SetItemTooltip("%s", tip);
    };
    auto range = [](const char* label, glm::vec2& v, float lo, float hi, const char* tip)
    {
        ImGui::SliderFloat2(label, &v.x, lo, hi, "%.2f");
        ImGui::SetItemTooltip("%s", tip);
    };

    if (ImGui::TreeNode("Rock"))
    {
        color("Rock Dark", s.rock.colorA, "Rock colour at the dark end of the grain");
        color("Rock Light", s.rock.colorB, "Rock colour at the light end of the grain");
        color("Streak Color", s.rock.streakColor, "Colour of the iron-like streaks in the rock");
        value("Streak Amount", s.rock.streakAmount, 0.0f, 1.0f, "How strongly the streaks show");
        value("Roughness Dark", s.rock.roughnessA, 0.0f, 1.0f, "Roughness of the dark rock (1 = matte)");
        value("Roughness Light", s.rock.roughnessB, 0.0f, 1.0f, "Roughness of the light rock (1 = matte)");
        value("Rock Slope", s.rock.slope, 0.0f, 1.0f, "Steepness where rock replaces ground: lower = more rock");
        value("Slope Noise", s.rock.slopeNoise, 0.0f, 0.6f, "How much that threshold wanders, so the edge is not clean");
        value("Rock Softness", s.rock.softness, 0.0f, 0.4f, "Width of the blend between ground and rock");
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Sand"))
    {
        color("Sand Dark", s.sand.colorA, "Sand colour at the dark end of the grain");
        color("Sand Light", s.sand.colorB, "Sand colour at the light end of the grain");
        value("Sand Roughness", s.sand.roughness, 0.0f, 1.0f, "Roughness of sand (1 = matte)");
        range("Aridity", s.sand.aridity, 0.0f, 1.0f, "Dryness range where beaches and deserts turn to sand");
        range("Beach Height", s.sand.beachHeight, 0.0f, 1.0f, "Sand fades out over this height range");
        range("Desert Aridity", s.sand.desertAridity, 0.0f, 1.0f, "Dryness range where the sand texture takes over");
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Snow"))
    {
        color("Snow Dark", s.snow.colorA, "Snow colour at the dark end of the grain");
        color("Snow Light", s.snow.colorB, "Snow colour at the light end of the grain");
        value("Snow Roughness", s.snow.roughness, 0.0f, 1.0f, "Roughness of snow (lower = more glint)");
        value("Cold Lowering", s.snow.coldLowering, 0.0f, 0.3f, "How far cold climates pull the snow line down");
        value("Blend Below", s.snow.blendBelow, 0.0f, 0.5f, "Snow fades in this far below the snow line");
        value("Blend Above", s.snow.blendAbove, 0.0f, 0.5f, "Snow is full this far above the snow line");
        value("Polar Temperature", s.snow.polarTemperature, 0.0f, 0.3f, "Temperature below which the poles freeze");
        value("Polar Amount", s.snow.polarAmount, 0.0f, 1.0f, "How white the frozen poles get");
        range("Slope Shed", s.snow.slopeShed, 0.0f, 1.0f, "Steepness range where snow slides off");
        value("Shed Amount", s.snow.shedAmount, 0.0f, 1.0f, "How much snow steep slopes lose");
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Ground"))
    {
        range("Lightness", s.ground.lightness, 0.0f, 2.0f, "Patchy lightness range of the biome colour");
        value("Ground Roughness", s.ground.roughness, 0.0f, 1.0f, "Roughness of the flat ground");
        range("Soil Dryness", s.ground.soilDryness, 0.0f, 1.0f, "Dryness range where bare soil replaces grass");
        value("Soil Amount", s.ground.soilAmount, 0.0f, 1.0f, "Most of the flat ground that soil can cover");
        value("Polar Soil", s.ground.polarSoil, 0.0f, 1.0f, "Bare soil in the cold tundra band");
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Detail"))
    {
        range("Grain", s.detail.grain, 0.0f, 2.0f, "Range of the fine albedo grain on every material");
        value("Grain Snow Fade", s.detail.grainSnowFade, 0.0f, 1.0f, "How much smoother snow is than other ground");
        value("Grain Frequency", s.detail.fineFrequency, 0.5f, 12.0f, "Size of the grain: higher = finer");
        value("Patch Frequency", s.detail.mediumFrequency, 0.1f, 4.0f, "Size of the patchy variation: higher = smaller");
        value("Bump Frequency", s.detail.bumpFrequency, 5.0f, 150.0f, "Size of the fine bumps: higher = finer");
        value("Bump Tilt", s.detail.bumpTilt, 0.0f, 0.05f, "How strongly the fine bumps tilt the light");
        value("Bump Snow Fade", s.detail.bumpSnowFade, 0.0f, 1.0f, "How much flatter snow is");
        value("Texture Bump", s.detail.textureBump, 0.0f, 2.0f, "Tilt from the material texture normal maps");
        value("Roughness Floor", s.detail.roughnessFloor, 0.0f, 1.0f, "Ground never gets glossier than this");
        value("Rock Roughness Floor", s.detail.matteRoughnessFloor, 0.0f, 1.0f,
              "Rock and snow never get glossier than this");
        value("Gamma", s.detail.gamma, 0.5f, 1.5f, "Below 1 lifts shadows and midtones");
        value("Temperature Noise", s.detail.temperatureNoise, 0.0f, 0.3f, "Noise on the climate band borders");
        value("Moisture Noise", s.detail.moistureNoise, 0.0f, 0.5f, "Patchy noise on moisture (biome borders)");
        value("Moisture Detail", s.detail.moistureDetailNoise, 0.0f, 0.5f, "Fine noise on moisture");
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Textures"))
    {
        const char* names[SurfaceSettings::TextureCount] = {"Rock", "Grass", "Sand", "Snow", "Soil"};
        for (int i = 0; i < SurfaceSettings::TextureCount; ++i)
        {
            ImGui::PushID(i);
            ImGui::SeparatorText(names[i]);
            auto& t = s.textures[i];
            value("Fine Tile", t.fineTile, 0.01f, 0.5f, "Size of the fine texture tile: larger = bigger features");
            value("Coarse Tile", t.coarseTile, 0.05f, 2.0f, "Size of the coarse tile that breaks up the repeat");
            value("Strength", t.strength, 0.0f, 2.0f, "How much the texture changes the colour");
            value("Normal Strength", t.normalStrength, 0.0f, 3.0f, "How much the texture tilts the light");
            ImGui::PopID();
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Strata"))
    {
        value("Strata Strength", s.strata.strength, 0.0f, 1.0f, "Rock layers on cliffs and crater walls: 0 = off");
        value("Bands", s.strata.bands, 2.0f, 200.0f, "Number of layers over the land height: higher = thinner layers");
        value("Warp", s.strata.warp, 0.0f, 3.0f, "How much the layers bend and wander along the cliff");
        color("Light Layer", s.strata.colorA, "Colour of the light rock layers");
        color("Dark Layer", s.strata.colorB, "Colour of the dark rock layers");
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Wetness"))
    {
        value("Wetness", s.wetness.strength, 0.0f, 1.0f, "Darker, glossier ground at coasts and on wet land: 0 = off");
        value("Shore Width", s.wetness.shoreWidth, 0.0f, 0.4f, "How far above sea level the shore stays wet");
        value("Wet Moisture", s.wetness.moisture, 0.0f, 1.0f, "Moisture above which inland ground turns wet (marshes, rain forest floor)");
        value("Darkening", s.wetness.darkening, 0.0f, 0.9f, "How much darker wet ground gets");
        value("Wet Roughness", s.wetness.roughness, 0.05f, 1.0f, "Roughness of wet ground: lower = more sun glint");
        color("Wet Tint", s.wetness.tint, "Colour cast of wet ground, for stains like acid or brine");
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Sediment"))
    {
        value("Sediment", s.sediment.strength, 0.0f, 1.0f, "Fine fill on low, flat ground (crater floors, basins, slope feet): 0 = off");
        color("Sediment Color", s.sediment.color, "Colour of the fill: sand, mud, salt, sulfur");
        range("Height Range", s.sediment.height, 0.0f, 1.0f, "Full below the first value, gone above the second (land height 0-1)");
        range("Slope Range", s.sediment.slope, 0.0f, 0.5f, "Only on flat ground: full below the first steepness, gone above the second");
        value("Patchiness", s.sediment.patchiness, 0.0f, 1.0f, "0 = an even sheet, 1 = broken patches");
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Vegetation"))
    {
        value("Treeline Strength", s.vegetation.strength, 0.0f, 1.0f, "Vegetation thins on steep ground and above the treeline: 0 = off");
        range("Slope Limit", s.vegetation.slope, 0.0f, 0.6f, "Vegetation thins out over this steepness range");
        range("Treeline", s.vegetation.treeline, 0.0f, 1.0f, "Vegetation thins out over this land height range (0-1)");
        color("Bare Ground", s.vegetation.bareColor, "Colour of the ground where vegetation is gone");
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Cover Layer"))
    {
        value("Cover Strength", s.cover.strength, 0.0f, 1.0f, "Ash, frost or dust over the ground: 0 = off");
        color("Cover Color", s.cover.color, "Colour of the cover: dark ash, white frost, ochre dust");
        value("Cover Roughness", s.cover.roughness, 0.05f, 1.0f, "Roughness of the cover: frost glints, ash and dust stay matte");
        range("Cover Slope", s.cover.slope, 0.0f, 1.0f, "The cover slides off steep ground over this steepness range");
        range("Cover Height", s.cover.height, 0.0f, 1.0f, "Land height range (0-1) the cover lies in");
        value("Facing", s.cover.facing, -1.0f, 1.0f, "+1 = only on slopes facing the sun, -1 = only on shaded slopes (frost), 0 = everywhere");
        value("Follow Sun", s.cover.sunFollow, -1.0f, 1.0f, "+1 = grows toward the point under the sun (dust), -1 = toward the night side (frost); for tidally locked worlds");
        value("Cover Patchiness", s.cover.patchiness, 0.0f, 1.0f, "0 = an even sheet, 1 = broken patches");
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Seafloor"))
    {
        color("Shallow Floor", s.seafloor.shallowColor, "Ground colour just below sea level");
        color("Deep Floor", s.seafloor.deepColor, "Ground colour deep below sea level");
        value("Depth Range", s.seafloor.depthRange, 0.005f, 0.3f, "Depth over which the floor goes from shallow to deep");
        ImGui::TreePop();
    }
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
