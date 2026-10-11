#include "SurfaceTab.h"
#include "Widgets.h"
#include "../BodyRuntime.h"
#include "../settings/SurfaceSettings.h"
#include <imgui.h>

namespace planets::render
{

bool SurfaceTab::Draw(BodyRuntime& body, SurfaceSettings& s)
{
    using namespace ui;
    bool regen = false;
    auto& sh = body.Config().shading;

    if (ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen))
    {
        Color("Rock Tint", s.rock.tint, "Multiplies the rock colour on slopes. White = unchanged.");
        Color("Sand Tint", s.sand.tint, "Multiplies beaches, deserts and dunes. White = unchanged.");
        Color("Snow Tint", s.snow.tint, "Multiplies snow and ice caps. White = unchanged.");
        Slider("Vegetation", s.ground.vegetation, 0.0f, 1.0f, "%.2f",
               "Grass texture on the flats. 0 = bare soil, 1 = full grass.");
        Slider("Sand Cover", s.sand.cover, 0.0f, 1.0f, "%.2f", "Sand on every flat. 1 = a dune sea.");
    }

    if (ImGui::CollapsingHeader("Climate", ImGuiTreeNodeFlags_DefaultOpen))
    {
        Slider("Snow Line", sh.snowLine, 0.0f, 1.5f, "%.2f",
               "Land height (0-1) where snow starts. Cold climates pull it lower. Above 1 = no snow.");
        Slider("Polar Caps", s.climate.polarCaps, 0.0f, 0.8f, "%.2f",
               "Ice caps from each pole, also on hot worlds. 0 = climate only.");
        Slider("Tidal Lock", s.climate.substellar, 0.0f, 1.0f, "%.2f",
               "One side always faces the star: hot day side, frozen night side. 0 = off.");
        Slider("City Coverage", s.climate.cityCoverage, 0.0f, 1.0f, "%.2f",
               "Share of land that is city at night. Light > Night Lights sets brightness.");
        if (ImGui::TreeNode("Climate Model"))
        {
            regen |= Check("Climate Model##on", sh.useClimateModel,
                           "Temperature and moisture from latitude, height and coasts. Off = flat colour.");
            ImGui::BeginDisabled(!sh.useClimateModel);
            regen |= SliderRegen("Large Scale", sh.largeNoiseScale, 0.2f, 0.7f, "%.2f",
                                 "Size of the large climate regions.");
            regen |= SliderRegen("Large Octaves", sh.largeNoiseOctaves, 2, 4, "Detail of the climate regions.");
            regen |= SliderRegen("Lapse Rate", sh.temperatureLapseRate, 0.5f, 4.0f, "%.1f",
                                 "How much colder it gets with altitude.");
            regen |= SliderRegen("Pole Falloff", sh.temperatureExponent, 0.3f, 1.5f, "%.2f",
                                 "How fast it gets colder toward the poles.");
            regen |= SliderRegen("Moisture Scale", sh.moistureNoiseScale, 0.5f, 3.0f, "%.1f",
                                 "Size of wet and dry regions.");
            regen |= SliderRegen("Moisture Variation", sh.moistureNoiseStrength, 0.0f, 0.4f, "%.2f",
                                 "How patchy moisture is across a region.");
            regen |= SliderRegen("Rain Bands", sh.hadleyIntensity, 0.0f, 2.0f, "%.1f",
                                 "Latitude rain bands: wet tropics, dry subtropics. 0 = none.");
            regen |= SliderRegen("Continentality", sh.continentalityStrength, 0.0f, 0.6f, "%.2f",
                                 "How much wetter coasts are than high interiors.");
            ImGui::EndDisabled();
            ImGui::TreePop();
        }
    }

    if (ImGui::CollapsingHeader("Surface Rules"))
    {
        if (ImGui::TreeNode("Rock"))
        {
            Color("Rock Dark", s.rock.colorA, "Rock colour at the dark end of the grain.");
            Color("Rock Light", s.rock.colorB, "Rock colour at the light end of the grain.");
            Color("Streak Colour", s.rock.streakColor, "Colour of the iron-like streaks in the rock.");
            Slider("Streak Amount", s.rock.streakAmount, 0.0f, 1.0f, "%.2f", "How strongly the streaks show.");
            Slider("Roughness Dark", s.rock.roughnessA, 0.0f, 1.0f, "%.2f", "Roughness of the dark rock. 1 = matte.");
            Slider("Roughness Light", s.rock.roughnessB, 0.0f, 1.0f, "%.2f", "Roughness of the light rock. 1 = matte.");
            Slider("Rock Slope", s.rock.slope, 0.0f, 1.0f, "%.2f", "Steepness where rock takes over. Lower = more rock.");
            Slider("Slope Noise", s.rock.slopeNoise, 0.0f, 0.6f, "%.2f", "How much that edge wanders.");
            Slider("Rock Softness", s.rock.softness, 0.0f, 0.4f, "%.2f", "Width of the ground-to-rock blend.");
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Sand"))
        {
            Color("Sand Dark", s.sand.colorA, "Sand colour at the dark end of the grain.");
            Color("Sand Light", s.sand.colorB, "Sand colour at the light end of the grain.");
            Slider("Sand Roughness", s.sand.roughness, 0.0f, 1.0f, "%.2f", "Roughness of sand. 1 = matte.");
            Range("Aridity", s.sand.aridity, 0.0f, 1.0f, "Dryness range where beaches and deserts turn to sand.");
            Range("Beach Height", s.sand.beachHeight, 0.0f, 1.0f, "Sand fades out over this land height range.");
            Range("Desert Aridity", s.sand.desertAridity, 0.0f, 1.0f, "Dryness range where the sand texture takes over.");
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Snow"))
        {
            Color("Snow Dark", s.snow.colorA, "Snow colour at the dark end of the grain.");
            Color("Snow Light", s.snow.colorB, "Snow colour at the light end of the grain.");
            Slider("Snow Roughness", s.snow.roughness, 0.0f, 1.0f, "%.2f", "Roughness of snow. Lower = more glint.");
            Slider("Cold Lowering", s.snow.coldLowering, 0.0f, 0.3f, "%.2f",
                   "How far cold climates pull the snow line (Climate) down.");
            Slider("Blend Below", s.snow.blendBelow, 0.0f, 0.5f, "%.2f", "Snow fades in this far below the snow line.");
            Slider("Blend Above", s.snow.blendAbove, 0.0f, 0.5f, "%.2f", "Snow is full this far above the snow line.");
            Slider("Polar Temperature", s.snow.polarTemperature, 0.0f, 0.3f, "%.2f",
                   "Temperature below which the poles freeze.");
            Slider("Polar Amount", s.snow.polarAmount, 0.0f, 1.0f, "%.2f", "How white the frozen poles get.");
            Range("Slope Shed", s.snow.slopeShed, 0.0f, 1.0f, "Steepness range where snow slides off.");
            Slider("Shed Amount", s.snow.shedAmount, 0.0f, 1.0f, "%.2f", "How much snow steep slopes lose.");
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Ground"))
        {
            Range("Lightness", s.ground.lightness, 0.0f, 2.0f, "Patchy lightness range of the biome colour.");
            Slider("Ground Roughness", s.ground.roughness, 0.0f, 1.0f, "%.2f", "Roughness of flat ground. 1 = matte.");
            Range("Soil Dryness", s.ground.soilDryness, 0.0f, 1.0f, "Dryness range where bare soil replaces grass.");
            Slider("Soil Amount", s.ground.soilAmount, 0.0f, 1.0f, "%.2f", "Most of the flat ground soil can cover.");
            Slider("Polar Soil", s.ground.polarSoil, 0.0f, 1.0f, "%.2f", "Bare soil in the cold tundra band.");
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Strata"))
        {
            Slider("Strata", s.strata.strength, 0.0f, 1.0f, "%.2f", "Rock layers on cliffs and crater walls. 0 = off.");
            Slider("Bands", s.strata.bands, 2.0f, 200.0f, "%.0f", "Layers over the land height. Higher = thinner.");
            Slider("Warp", s.strata.warp, 0.0f, 3.0f, "%.2f", "How much the layers bend along the cliff.");
            Color("Light Layer", s.strata.colorA, "Colour of the light rock layers.");
            Color("Dark Layer", s.strata.colorB, "Colour of the dark rock layers.");
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Wet Ground"))
        {
            Slider("Wet Ground", s.wetness.strength, 0.0f, 1.0f, "%.2f",
                   "Darker, glossier ground at coasts and on wet land. 0 = off.");
            Slider("Shore Width", s.wetness.shoreWidth, 0.0f, 0.4f, "%.2f", "How far above sea level the shore stays wet.");
            Slider("Wet Moisture", s.wetness.moisture, 0.0f, 1.0f, "%.2f", "Moisture above which inland ground is wet.");
            Slider("Darkening", s.wetness.darkening, 0.0f, 0.9f, "%.2f", "How much darker wet ground gets.");
            Slider("Wet Roughness", s.wetness.roughness, 0.05f, 1.0f, "%.2f", "Roughness of wet ground. Lower = glint.");
            Color("Wet Tint", s.wetness.tint, "Colour cast of wet ground: acid or brine stains.");
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Sediment"))
        {
            Slider("Sediment", s.sediment.strength, 0.0f, 1.0f, "%.2f",
                   "Fine fill on low, flat ground: crater floors, basins. 0 = off.");
            Color("Sediment Colour", s.sediment.color, "Colour of the fill: sand, mud, salt, sulfur.");
            Range("Height Range", s.sediment.height, 0.0f, 1.0f, "Full below the first land height, gone above the second.");
            Range("Slope Range", s.sediment.slope, 0.0f, 0.5f, "Full below the first steepness, gone above the second.");
            Slider("Patchiness", s.sediment.patchiness, 0.0f, 1.0f, "%.2f", "0 = an even sheet, 1 = broken patches.");
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Treeline"))
        {
            Slider("Treeline", s.vegetation.strength, 0.0f, 1.0f, "%.2f",
                   "Vegetation thins on steep and high ground. 0 = off.");
            Range("Slope Limit", s.vegetation.slope, 0.0f, 0.6f, "Vegetation thins out over this steepness range.");
            Range("Height Limit", s.vegetation.treeline, 0.0f, 1.0f, "Vegetation thins out over this land height range.");
            Color("Bare Ground", s.vegetation.bareColor, "Colour where the vegetation is gone.");
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Cover"))
        {
            Slider("Cover", s.cover.strength, 0.0f, 1.0f, "%.2f", "Ash, frost or dust over the ground. 0 = off.");
            Color("Cover Colour", s.cover.color, "Dark ash, white frost, ochre dust.");
            Slider("Cover Roughness", s.cover.roughness, 0.05f, 1.0f, "%.2f", "Frost glints; ash and dust stay matte.");
            Range("Cover Slope", s.cover.slope, 0.0f, 1.0f, "The cover slides off steep ground over this range.");
            Range("Cover Height", s.cover.height, 0.0f, 1.0f, "Land height range (0-1) the cover lies in.");
            Slider("Facing", s.cover.facing, -1.0f, 1.0f, "%.2f",
                   "+1 = sunny slopes only, -1 = shaded slopes only, 0 = everywhere.");
            Slider("Follow Sun", s.cover.sunFollow, -1.0f, 1.0f, "%.2f",
                   "+1 = toward the point under the sun, -1 = toward the night side.");
            Slider("Cover Patchiness", s.cover.patchiness, 0.0f, 1.0f, "%.2f", "0 = an even sheet, 1 = broken patches.");
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Fine Detail"))
        {
            auto& d = s.detail;
            Slider("AO Strength", sh.aoStrength, 0.0f, 1.0f, "%.2f", "Darkening of creases and valleys.");
            Range("Grain", d.grain, 0.0f, 2.0f, "Range of the fine colour grain on every material.");
            Slider("Grain Snow Fade", d.grainSnowFade, 0.0f, 1.0f, "%.2f", "How much smoother snow is.");
            Slider("Grain Frequency", d.fineFrequency, 0.5f, 12.0f, "%.1f", "Size of the grain. Higher = finer.");
            Slider("Patch Frequency", d.mediumFrequency, 0.1f, 4.0f, "%.2f", "Size of the patchy variation.");
            Slider("Bump Frequency", d.bumpFrequency, 5.0f, 150.0f, "%.0f", "Size of the fine bumps. Higher = finer.");
            Slider("Bump Tilt", d.bumpTilt, 0.0f, 0.05f, "%.3f", "How strongly the fine bumps tilt the light.");
            Slider("Bump Snow Fade", d.bumpSnowFade, 0.0f, 1.0f, "%.2f", "How much flatter snow is.");
            Slider("Texture Bump", d.textureBump, 0.0f, 2.0f, "%.2f", "Tilt from the texture normal maps.");
            Slider("Roughness Floor", d.roughnessFloor, 0.0f, 1.0f, "%.2f", "Ground never gets glossier than this.");
            Slider("Rock Roughness Floor", d.matteRoughnessFloor, 0.0f, 1.0f, "%.2f",
                   "Rock and snow never get glossier than this.");
            Slider("Gamma", d.gamma, 0.5f, 1.5f, "%.2f", "Below 1 lifts shadows and midtones.");
            Slider("Temperature Noise", d.temperatureNoise, 0.0f, 0.3f, "%.2f", "Noise on the climate band borders.");
            Slider("Moisture Noise", d.moistureNoise, 0.0f, 0.5f, "%.2f", "Patchy noise on biome borders.");
            Slider("Moisture Detail", d.moistureDetailNoise, 0.0f, 0.5f, "%.2f", "Fine noise on biome borders.");
            regen |= SliderRegen("Detail Scale", sh.detailNoiseScale, 0.5f, 8.0f, "%.1f",
                                 "Size of the computed detail noise. Rebuilds the terrain.");
            regen |= SliderRegen("Small Scale", sh.smallNoiseScale, 5.0f, 40.0f, "%.1f",
                                 "Size of the small computed noise. Rebuilds the terrain.");
            regen |= SliderRegen("Small Octaves", sh.smallNoiseOctaves, 2, 8, "Layers of small noise.");
            regen |= SliderRegen("Warp Strength", sh.warpStrength, 0.0f, 1.0f, "%.2f",
                                 "How much the detail noise swirls.");
            ImGui::TreePop();
        }
    }

    if (ImGui::CollapsingHeader("Textures"))
    {
        const char* names[SurfaceSettings::TextureCount] = {"Rock", "Grass", "Sand", "Snow", "Soil"};
        for (int i = 0; i < SurfaceSettings::TextureCount; ++i)
        {
            ImGui::PushID(i);
            ImGui::SeparatorText(names[i]);
            auto& t = s.textures[i];
            Slider("Tile Size", t.fineTile, 0.01f, 0.5f, "%.3f", "Size of the texture on the ground. Larger = bigger.");
            Slider("Coarse Tile", t.coarseTile, 0.05f, 2.0f, "%.2f", "Size of the second tile that hides the repeat.");
            Slider("Strength", t.strength, 0.0f, 2.0f, "%.2f", "How much the texture changes the colour.");
            Slider("Normal Strength", t.normalStrength, 0.0f, 3.0f, "%.2f", "How much the texture tilts the light.");
            ImGui::PopID();
        }
    }

    return regen;
}

} // namespace planets::render
