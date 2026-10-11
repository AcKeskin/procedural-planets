#include "BodyLook.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

namespace planets::render::types
{

namespace
{
using nlohmann::json;

void Read(const json& j, const char* key, float& out)
{
    if (j.contains(key) && j[key].is_number())
        out = j[key].get<float>();
}

void Read(const json& j, const char* key, int& out)
{
    if (j.contains(key) && j[key].is_number_integer())
        out = j[key].get<int>();
}

void Read(const json& j, const char* key, bool& out)
{
    if (j.contains(key) && j[key].is_boolean())
        out = j[key].get<bool>();
}

void Read(const json& j, const char* key, glm::vec3& out)
{
    if (j.contains(key) && j[key].is_array() && j[key].size() == 3)
        out = glm::vec3(j[key][0].get<float>(), j[key][1].get<float>(), j[key][2].get<float>());
}

const json& Block(const json& j, const char* key)
{
    static const json empty = json::object();
    return j.contains(key) && j[key].is_object() ? j[key] : empty;
}

void Read(const json& j, const char* key, glm::vec2& out)
{
    if (j.contains(key) && j[key].is_array() && j[key].size() == 2 && j[key][0].is_number() && j[key][1].is_number())
        out = glm::vec2(j[key][0].get<float>(), j[key][1].get<float>());
}

// render.surface: every rule of the surface shader, grouped like SurfaceSettings.
void ReadSurface(const json& j, SurfaceSettings& s)
{
    const json& r = Block(j, "rock");
    Read(r, "colorA", s.rock.colorA);
    Read(r, "colorB", s.rock.colorB);
    Read(r, "roughnessA", s.rock.roughnessA);
    Read(r, "roughnessB", s.rock.roughnessB);
    Read(r, "streakColor", s.rock.streakColor);
    Read(r, "streakAmount", s.rock.streakAmount);
    Read(r, "tint", s.rock.tint);
    Read(r, "slope", s.rock.slope);
    Read(r, "slopeNoise", s.rock.slopeNoise);
    Read(r, "softness", s.rock.softness);

    const json& a = Block(j, "sand");
    Read(a, "colorA", s.sand.colorA);
    Read(a, "colorB", s.sand.colorB);
    Read(a, "roughness", s.sand.roughness);
    Read(a, "tint", s.sand.tint);
    Read(a, "cover", s.sand.cover);
    Read(a, "aridity", s.sand.aridity);
    Read(a, "beachHeight", s.sand.beachHeight);
    Read(a, "desertAridity", s.sand.desertAridity);

    const json& n = Block(j, "snow");
    Read(n, "colorA", s.snow.colorA);
    Read(n, "colorB", s.snow.colorB);
    Read(n, "roughness", s.snow.roughness);
    Read(n, "tint", s.snow.tint);
    Read(n, "coldLowering", s.snow.coldLowering);
    Read(n, "blendBelow", s.snow.blendBelow);
    Read(n, "blendAbove", s.snow.blendAbove);
    Read(n, "polarTemperature", s.snow.polarTemperature);
    Read(n, "polarAmount", s.snow.polarAmount);
    Read(n, "slopeShed", s.snow.slopeShed);
    Read(n, "shedAmount", s.snow.shedAmount);

    const json& g = Block(j, "ground");
    Read(g, "lightness", s.ground.lightness);
    Read(g, "roughness", s.ground.roughness);
    Read(g, "vegetation", s.ground.vegetation);
    Read(g, "soilDryness", s.ground.soilDryness);
    Read(g, "soilAmount", s.ground.soilAmount);
    Read(g, "polarSoil", s.ground.polarSoil);

    const json& d = Block(j, "detail");
    Read(d, "grain", s.detail.grain);
    Read(d, "grainSnowFade", s.detail.grainSnowFade);
    Read(d, "mediumFrequency", s.detail.mediumFrequency);
    Read(d, "fineFrequency", s.detail.fineFrequency);
    Read(d, "bumpFrequency", s.detail.bumpFrequency);
    Read(d, "bumpTilt", s.detail.bumpTilt);
    Read(d, "bumpSnowFade", s.detail.bumpSnowFade);
    Read(d, "textureBump", s.detail.textureBump);
    Read(d, "roughnessFloor", s.detail.roughnessFloor);
    Read(d, "matteRoughnessFloor", s.detail.matteRoughnessFloor);
    Read(d, "gamma", s.detail.gamma);
    Read(d, "temperatureNoise", s.detail.temperatureNoise);
    Read(d, "moistureNoise", s.detail.moistureNoise);
    Read(d, "moistureDetailNoise", s.detail.moistureDetailNoise);

    const json& t = Block(j, "textures");
    const char* layers[SurfaceSettings::TextureCount] = {"rock", "grass", "sand", "snow", "soil"};
    for (int i = 0; i < SurfaceSettings::TextureCount; ++i)
    {
        const json& l = Block(t, layers[i]);
        Read(l, "fineTile", s.textures[i].fineTile);
        Read(l, "coarseTile", s.textures[i].coarseTile);
        Read(l, "strength", s.textures[i].strength);
        Read(l, "normalStrength", s.textures[i].normalStrength);
    }

    const json& f = Block(j, "seafloor");
    Read(f, "shallowColor", s.seafloor.shallowColor);
    Read(f, "deepColor", s.seafloor.deepColor);
    Read(f, "depthRange", s.seafloor.depthRange);

    const json& strataJ = Block(j, "strata");
    Read(strataJ, "strength", s.strata.strength);
    Read(strataJ, "bands", s.strata.bands);
    Read(strataJ, "warp", s.strata.warp);
    Read(strataJ, "colorA", s.strata.colorA);
    Read(strataJ, "colorB", s.strata.colorB);

    const json& c = Block(j, "climate");
    Read(c, "polarCaps", s.climate.polarCaps);
    Read(c, "substellar", s.climate.substellar);
    Read(c, "cityCoverage", s.climate.cityCoverage);
}

std::string Name(const json& j, const char* key)
{
    return j.contains(key) && j[key].is_string() ? j[key].get<std::string>() : std::string();
}

// Star colour by class, so a body can say "red" instead of an rgb. Unknown names keep the default.
void ApplyStarClass(const std::string& name, BodyLook& look)
{
    if (name == "red")
        look.sunColor = glm::vec3(1.0f, 0.55f, 0.32f);
    else if (name == "orange")
        look.sunColor = glm::vec3(1.0f, 0.78f, 0.55f);
    else if (name == "white")
        look.sunColor = glm::vec3(1.0f, 1.0f, 1.0f);
    else if (name == "blue")
        look.sunColor = glm::vec3(0.72f, 0.82f, 1.0f);
}

// Weather is a starting point for haze and clouds; the explicit atmosphere and cloud fields
// read after it still win.
void ApplyWeather(const std::string& name, BodyLook& look)
{
    auto& a = look.atmosphere;
    auto& c = look.clouds;
    if (name == "dust")
    {
        a.mieCoefficient = 10.0f;
        a.mieDensityFalloff = 6.0f;
        a.hazeColor = glm::vec3(0.95f, 0.68f, 0.40f);
        c.coverage = 0.35f;
        c.color = glm::vec3(0.95f, 0.80f, 0.62f);
    }
    else if (name == "spores")
    {
        a.mieCoefficient = 6.0f;
        a.mieDensityFalloff = 10.0f;
        a.hazeColor = glm::vec3(0.70f, 0.95f, 0.50f);
    }
    else if (name == "fog")
    {
        // Thin layer at sea level: shows over low land and lakes, sits under hills.
        a.mieCoefficient = 12.0f;
        a.mieDensityFalloff = 25.0f;
        a.hazeColor = glm::vec3(0.85f, 0.90f, 0.85f);
        c.altitude = 0.012f;
    }
    else if (name == "storms")
    {
        a.mieCoefficient = 1.5f;
        c.coverage = 0.75f;
        c.density = 450.0f;
        c.color = glm::vec3(0.55f, 0.57f, 0.60f);
    }
}
} // namespace

BodyLook ParseBodyLook(const json& body)
{
    BodyLook look;
    const json& render = Block(body, "render");
    ApplyWeather(Name(render, "weather"), look);

    const json& a = Block(render, "atmosphere");
    Read(a, "enabled", look.atmosphere.enabled);
    Read(a, "scale", look.atmosphere.atmosphereScale);
    Read(a, "wavelengths", look.atmosphere.wavelengths);
    Read(a, "scatteringStrength", look.atmosphere.scatteringStrength);
    Read(a, "densityFalloff", look.atmosphere.densityFalloff);
    Read(a, "mieCoefficient", look.atmosphere.mieCoefficient);
    Read(a, "mieDensityFalloff", look.atmosphere.mieDensityFalloff);
    Read(a, "hazeColor", look.atmosphere.hazeColor);
    Read(a, "intensity", look.atmosphere.intensity);

    const json& c = Block(render, "clouds");
    Read(c, "enabled", look.clouds.enabled);
    Read(c, "coverage", look.clouds.coverage);
    Read(c, "density", look.clouds.density);
    Read(c, "scale", look.clouds.scale);
    Read(c, "altitude", look.clouds.altitude);
    Read(c, "thickness", look.clouds.thickness);
    Read(c, "speed", look.clouds.speed);
    Read(c, "shadowStrength", look.clouds.shadowStrength);
    Read(c, "color", look.clouds.color);

    const json& o = Block(render, "ocean");
    Read(o, "enabled", look.ocean.enabled);
    Read(o, "deepColor", look.ocean.deepColor);
    Read(o, "shallowColor", look.ocean.shallowColor);
    Read(o, "clarity", look.ocean.clarity);
    Read(o, "waveStrength", look.ocean.waveStrength);
    Read(o, "waveScale", look.ocean.waveScale);
    Read(o, "waveSpeed", look.ocean.waveSpeed);
    Read(o, "emissive", look.ocean.emissive);
    Read(o, "crust", look.ocean.crust);
    Read(o, "frozen", look.ocean.frozen);
    Read(o, "reflectivity", look.ocean.reflectivity);
    Read(o, "foamColor", look.ocean.foamColor);
    Read(o, "crustColor", look.ocean.crustColor);

    const json& s = Block(render, "star");
    ApplyStarClass(Name(s, "class"), look);
    Read(s, "color", look.sunColor);
    Read(s, "intensity", look.lighting.sunIntensity);

    const json& l = Block(render, "lighting");
    Read(l, "ambient", look.lighting.ambientLight);
    Read(l, "nightLights", look.lighting.nightLightStrength);
    Read(l, "hazeColor", look.lighting.hazeColor);
    Read(l, "hazeStrength", look.lighting.hazeStrength);

    const json& m = Block(render, "materials");
    // Older keys from the planet-types render block; render.surface (below) wins.
    Read(m, "rockTint", look.surface.rock.tint);
    Read(m, "sandTint", look.surface.sand.tint);
    Read(m, "snowTint", look.surface.snow.tint);
    Read(m, "vegetation", look.surface.ground.vegetation);
    Read(m, "sandCover", look.surface.sand.cover);

    const json& k = Block(render, "climate");
    Read(k, "polarCaps", look.surface.climate.polarCaps);
    Read(k, "substellar", look.surface.climate.substellar);
    Read(k, "cityCoverage", look.surface.climate.cityCoverage);

    ReadSurface(Block(render, "surface"), look.surface);

    if (render.contains("palette") && render["palette"].is_array())
    {
        for (const auto& entry : render["palette"])
        {
            if (entry.is_array() && entry.size() == 3)
                look.palette.emplace_back(entry[0].get<float>(), entry[1].get<float>(), entry[2].get<float>());
        }
    }

    return look;
}

BodyLook LoadBodyLook(const std::string& path)
{
    std::ifstream in(path);
    const json body = json::parse(in, nullptr, false);
    if (body.is_discarded())
    {
        std::cerr << "[BodyLook] Cannot parse " << path << "; using the default look" << std::endl;
        return {};
    }
    return ParseBodyLook(body);
}

void ApplyBodyLook(const BodyLook& look,
                   effects::AtmosphereSettings& atmosphere,
                   effects::CloudSettings& clouds,
                   effects::OceanSettings& ocean,
                   SceneSettings& scene,
                   SurfaceSettings& surface)
{
    surface = look.surface;
    atmosphere = look.atmosphere;
    clouds = look.clouds;
    ocean = look.ocean;
    scene.sunColor = look.sunColor;
    scene.lighting.sunIntensity = look.lighting.sunIntensity;
    scene.lighting.ambientLight = look.lighting.ambientLight;
    scene.lighting.nightLightStrength = look.lighting.nightLightStrength;
    scene.lighting.hazeColor = look.lighting.hazeColor;
    scene.lighting.hazeStrength = look.lighting.hazeStrength;
}

} // namespace planets::render::types
