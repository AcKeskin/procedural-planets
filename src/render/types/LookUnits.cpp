#include "LookUnits.h"
#include "BodyLook.h"

#include "planetgen/model/BodyConfigJson.h"
#include "planetgen/model/BodyVariety.h"
#include <nlohmann/json.hpp>

namespace planets::render::types
{

namespace
{
bool Has(uint32_t mask, LookUnit u)
{
    return (mask & UnitBit(u)) != 0;
}

bool StartsWith(const std::string& s, const char* prefix)
{
    return s.rfind(prefix, 0) == 0;
}
} // namespace

void CopyUnits(uint32_t mask, const LookState& from, LookState& to)
{
    if (Has(mask, LookUnit::Terrain))
    {
        // A variety key can hit any metadata field except the name, type and sea level (Ocean).
        to.config.metadata.radius = from.config.metadata.radius;
        to.config.metadata.atmosphereHeight = from.config.metadata.atmosphereHeight;
        to.config.metadata.hasSolidSurface = from.config.metadata.hasSolidSurface;
        to.config.metadata.hasAtmosphere = from.config.metadata.hasAtmosphere;
        to.config.shape = from.config.shape;
        to.config.tectonics = from.config.tectonics;
        to.config.oceanFloor = from.config.oceanFloor;
        to.config.heightDetail = from.config.heightDetail;
        to.config.erosion = from.config.erosion;
        to.config.macroWorld = from.config.macroWorld;
        to.config.terrainStyle = from.config.terrainStyle;
    }
    if (Has(mask, LookUnit::Surface))
    {
        // Seabed and shadow fill sit in SurfaceSettings but belong to other tabs.
        const auto seafloor = to.surface.seafloor;
        const auto skyAmbient = to.surface.skyAmbient;
        to.surface = from.surface;
        to.surface.seafloor = seafloor;
        to.surface.skyAmbient = skyAmbient;
        to.config.shading = from.config.shading;
        to.palette = from.palette;
    }
    if (Has(mask, LookUnit::Ocean))
    {
        to.config.metadata.seaLevel = from.config.metadata.seaLevel;
        to.ocean = from.ocean;
        to.surface.seafloor = from.surface.seafloor;
    }
    if (Has(mask, LookUnit::Sky))
    {
        to.atmosphere = from.atmosphere;
        to.clouds = from.clouds;
        to.scene.lighting.hazeStrength = from.scene.lighting.hazeStrength;
        to.scene.lighting.hazeColor = from.scene.lighting.hazeColor;
    }
    if (Has(mask, LookUnit::Light))
    {
        const float hazeStrength = to.scene.lighting.hazeStrength;
        const glm::vec3 hazeColor = to.scene.lighting.hazeColor;
        to.scene = from.scene;
        to.scene.lighting.hazeStrength = hazeStrength;
        to.scene.lighting.hazeColor = hazeColor;
        to.surface.skyAmbient = from.surface.skyAmbient;
    }
}

MergedRoll MergeRoll(const LookState& rolled, const LookState& current, const LookState& baseline, uint32_t keptMask)
{
    MergedRoll m{rolled, rolled};
    CopyUnits(keptMask, current, m.next);
    CopyUnits(keptMask, baseline, m.baseline);
    return m;
}

LookUnit UnitOfKey(const std::string& key)
{
    if (key == "metadata.seaLevel" || StartsWith(key, "render.ocean.") || StartsWith(key, "render.surface.seafloor."))
        return LookUnit::Ocean;
    if (StartsWith(key, "render.atmosphere.") || StartsWith(key, "render.clouds.") || key == "render.weather" ||
        StartsWith(key, "render.lighting.haze"))
        return LookUnit::Sky;
    if (StartsWith(key, "render.star.") || StartsWith(key, "render.lighting.") ||
        StartsWith(key, "render.surface.skyAmbient."))
        return LookUnit::Light;
    if (StartsWith(key, "render.") || StartsWith(key, "shading."))
        return LookUnit::Surface;
    return LookUnit::Terrain;
}

uint32_t VarietyUnits(const nlohmann::json& body)
{
    uint32_t mask = 0;
    if (!planetgen::HasVariety(body))
        return mask;
    for (const auto& item : body["variety"].items())
        mask |= UnitBit(UnitOfKey(item.key()));
    return mask;
}

std::string RollLook(const nlohmann::json& body, uint32_t seed, const SceneSettings& scene, LookState& out)
{
    const nlohmann::json world = planetgen::RollVariety(body, seed);
    LookState state;
    const std::string err = planetgen::BodyConfigFromJson(world.dump(), state.config);
    if (!err.empty())
        return err;

    const BodyLook look = ParseBodyLook(world);
    state.scene = scene;
    ApplyBodyLook(look, state.atmosphere, state.clouds, state.ocean, state.scene, state.surface);
    state.atmosphere.enabled = state.config.metadata.hasAtmosphere && look.atmosphere.enabled;
    state.palette = look.palette;
    out = std::move(state);
    return {};
}

} // namespace planets::render::types
