#include "ParamRegistry.h"

#include "planetgen/model/BodyConfig.h"

#include <functional>
#include <unordered_map>

namespace planets::app::instrument
{
namespace
{

// --- value coercion: JSON gives us float/int/bool/vec3; targets want a specific type ---

bool AsFloat(const ParamValue& v, float& out)
{
    if (auto* f = std::get_if<float>(&v))
    {
        out = *f;
        return true;
    }
    if (auto* i = std::get_if<int>(&v))
    {
        out = static_cast<float>(*i);
        return true;
    }
    if (auto* b = std::get_if<bool>(&v))
    {
        out = *b ? 1.0f : 0.0f;
        return true;
    }
    return false;
}

bool AsInt(const ParamValue& v, int& out)
{
    if (auto* i = std::get_if<int>(&v))
    {
        out = *i;
        return true;
    }
    if (auto* f = std::get_if<float>(&v))
    {
        out = static_cast<int>(*f);
        return true;
    }
    return false;
}

bool AsBool(const ParamValue& v, bool& out)
{
    if (auto* b = std::get_if<bool>(&v))
    {
        out = *b;
        return true;
    }
    if (auto* i = std::get_if<int>(&v))
    {
        out = (*i != 0);
        return true;
    }
    if (auto* f = std::get_if<float>(&v))
    {
        out = (*f != 0.0f);
        return true;
    }
    return false;
}

bool AsVec3(const ParamValue& v, glm::vec3& out)
{
    if (auto* vv = std::get_if<glm::vec3>(&v))
    {
        out = *vv;
        return true;
    }
    return false;
}

// A setter writes value into targets; returns false if the value can't coerce or the
// required target (e.g. body) is missing.
using Setter = std::function<bool(const ParamTargets&, const ParamValue&)>;

struct Entry
{
    bool needsRegen;
    Setter set;
};

// Field-setter factories keep the table terse: F_FLOAT(scene->lighting.sunIntensity) etc.
// Each captures a member pointer chain via a lambda over targets.
#define SET_FLOAT(EXPR)                                                                                                \
    [](const ParamTargets& t, const ParamValue& v) -> bool                                                             \
    {                                                                                                                  \
        float f;                                                                                                       \
        if (!AsFloat(v, f))                                                                                            \
            return false;                                                                                              \
        EXPR = f;                                                                                                      \
        return true;                                                                                                   \
    }
#define SET_INT(EXPR)                                                                                                  \
    [](const ParamTargets& t, const ParamValue& v) -> bool                                                             \
    {                                                                                                                  \
        int i;                                                                                                         \
        if (!AsInt(v, i))                                                                                              \
            return false;                                                                                              \
        EXPR = i;                                                                                                      \
        return true;                                                                                                   \
    }
#define SET_BOOL(EXPR)                                                                                                 \
    [](const ParamTargets& t, const ParamValue& v) -> bool                                                             \
    {                                                                                                                  \
        bool b;                                                                                                        \
        if (!AsBool(v, b))                                                                                             \
            return false;                                                                                              \
        EXPR = b;                                                                                                      \
        return true;                                                                                                   \
    }
#define SET_VEC3(EXPR)                                                                                                 \
    [](const ParamTargets& t, const ParamValue& v) -> bool                                                             \
    {                                                                                                                  \
        glm::vec3 vv;                                                                                                  \
        if (!AsVec3(v, vv))                                                                                            \
            return false;                                                                                              \
        EXPR = vv;                                                                                                     \
        return true;                                                                                                   \
    }

// Guard a surface.* setter: reject when no surface settings are wired.
#define SURF_FLOAT(EXPR)                                                                                                   [](const ParamTargets& t, const ParamValue& v) -> bool                                                                 {                                                                                                                          float f;                                                                                                               if (!t.surface || !AsFloat(v, f))                                                                                          return false;                                                                                                      EXPR = f;                                                                                                              return true;                                                                                                       }
#define SURF_VEC3(EXPR)                                                                                                    [](const ParamTargets& t, const ParamValue& v) -> bool                                                                 {                                                                                                                          glm::vec3 vv;                                                                                                          if (!t.surface || !AsVec3(v, vv))                                                                                          return false;                                                                                                      EXPR = vv;                                                                                                             return true;                                                                                                       }

// Guard a body.* setter: reject when no body is loaded.
#define BODY_FLOAT(EXPR)                                                                                               \
    [](const ParamTargets& t, const ParamValue& v) -> bool                                                             \
    {                                                                                                                  \
        if (!t.body)                                                                                                   \
            return false;                                                                                              \
        float f;                                                                                                       \
        if (!AsFloat(v, f))                                                                                            \
            return false;                                                                                              \
        EXPR = f;                                                                                                      \
        return true;                                                                                                   \
    }
#define BODY_INT(EXPR)                                                                                                 \
    [](const ParamTargets& t, const ParamValue& v) -> bool                                                             \
    {                                                                                                                  \
        if (!t.body)                                                                                                   \
            return false;                                                                                              \
        int i;                                                                                                         \
        if (!AsInt(v, i))                                                                                              \
            return false;                                                                                              \
        EXPR = i;                                                                                                      \
        return true;                                                                                                   \
    }
#define BODY_BOOL(EXPR)                                                                                                \
    [](const ParamTargets& t, const ParamValue& v) -> bool                                                             \
    {                                                                                                                  \
        if (!t.body)                                                                                                   \
            return false;                                                                                              \
        bool b;                                                                                                        \
        if (!AsBool(v, b))                                                                                             \
            return false;                                                                                              \
        EXPR = b;                                                                                                      \
        return true;                                                                                                   \
    }
#define BODY_VEC3(EXPR)                                                                                                \
    [](const ParamTargets& t, const ParamValue& v) -> bool                                                             \
    {                                                                                                                  \
        if (!t.body)                                                                                                   \
            return false;                                                                                              \
        glm::vec3 vv;                                                                                                  \
        if (!AsVec3(v, vv))                                                                                            \
            return false;                                                                                              \
        EXPR = vv;                                                                                                     \
        return true;                                                                                                   \
    }

const std::unordered_map<std::string, Entry>& Table()
{
    // Built once. Class rule: read into RenderContext each frame OR pushed by
    // BodyRuntime::SetRenderUniforms (body.shading.*) → real-time (needsRegen=false);
    // consumed by RegenerateLodSystem (body.shape/tectonics/oceanFloor/heightDetail/erosion,
    // gen.seed, lod.*) → needsRegen=true.
    static const std::unordered_map<std::string, Entry> table = {
        // ---- scene (real-time) ----
        {"scene.lightDir", {false, SET_VEC3(t.scene->lightDir)}},
        {"scene.sunSize", {false, SET_FLOAT(t.scene->sunSize)}},
        {"scene.sunColor", {false, SET_VEC3(t.scene->sunColor)}},
        {"scene.starDensity", {false, SET_FLOAT(t.scene->starDensity)}},
        {"scene.exposure", {false, SET_FLOAT(t.scene->exposure)}},
        {"scene.bloomStrength", {false, SET_FLOAT(t.scene->bloomStrength)}},
        {"scene.lighting.sunIntensity", {false, SET_FLOAT(t.scene->lighting.sunIntensity)}},
        {"scene.lighting.ambientLight", {false, SET_FLOAT(t.scene->lighting.ambientLight)}},
        {"scene.lighting.specularStrength", {false, SET_FLOAT(t.scene->lighting.specularStrength)}},
        {"scene.lighting.specularPower", {false, SET_FLOAT(t.scene->lighting.specularPower)}},
        {"scene.lighting.detailFadeStart", {false, SET_FLOAT(t.scene->lighting.detailFadeStart)}},
        {"scene.lighting.hazeStrength", {false, SET_FLOAT(t.scene->lighting.hazeStrength)}},
        {"scene.lighting.hazeColor", {false, SET_VEC3(t.scene->lighting.hazeColor)}},

        // ---- ocean (real-time) ----
        {"ocean.enabled", {false, SET_BOOL(t.ocean->enabled)}},
        {"ocean.deepColor", {false, SET_VEC3(t.ocean->deepColor)}},
        {"ocean.shallowColor", {false, SET_VEC3(t.ocean->shallowColor)}},
        {"ocean.clarity", {false, SET_FLOAT(t.ocean->clarity)}},
        {"ocean.waveStrength", {false, SET_FLOAT(t.ocean->waveStrength)}},
        {"ocean.waveScale", {false, SET_FLOAT(t.ocean->waveScale)}},
        {"ocean.waveSpeed", {false, SET_FLOAT(t.ocean->waveSpeed)}},
        {"ocean.emissive", {false, SET_FLOAT(t.ocean->emissive)}},
        {"ocean.crust", {false, SET_FLOAT(t.ocean->crust)}},
        {"ocean.frozen", {false, SET_FLOAT(t.ocean->frozen)}},
        {"ocean.reflectivity", {false, SET_FLOAT(t.ocean->reflectivity)}},
        {"ocean.foamColor", {false, SET_VEC3(t.ocean->foamColor)}},
        {"ocean.crustColor", {false, SET_VEC3(t.ocean->crustColor)}},

        // ---- atmosphere (real-time) ----
        {"atmosphere.enabled", {false, SET_BOOL(t.atmosphere->enabled)}},
        {"atmosphere.atmosphereScale", {false, SET_FLOAT(t.atmosphere->atmosphereScale)}},
        {"atmosphere.wavelengths", {false, SET_VEC3(t.atmosphere->wavelengths)}},
        {"atmosphere.scatteringStrength", {false, SET_FLOAT(t.atmosphere->scatteringStrength)}},
        {"atmosphere.densityFalloff", {false, SET_FLOAT(t.atmosphere->densityFalloff)}},
        {"atmosphere.mieCoefficient", {false, SET_FLOAT(t.atmosphere->mieCoefficient)}},
        {"atmosphere.mieDensityFalloff", {false, SET_FLOAT(t.atmosphere->mieDensityFalloff)}},
        {"atmosphere.hazeColor", {false, SET_VEC3(t.atmosphere->hazeColor)}},
        {"atmosphere.numInScatteringPoints", {false, SET_INT(t.atmosphere->numInScatteringPoints)}},
        {"atmosphere.numOpticalDepthPoints", {false, SET_INT(t.atmosphere->numOpticalDepthPoints)}},
        {"atmosphere.intensity", {false, SET_FLOAT(t.atmosphere->intensity)}},
        {"atmosphere.ditherStrength", {false, SET_FLOAT(t.atmosphere->ditherStrength)}},
        {"atmosphere.ditherScale", {false, SET_FLOAT(t.atmosphere->ditherScale)}},

        // ---- clouds (real-time) ----
        {"clouds.enabled", {false, SET_BOOL(t.clouds->enabled)}},
        {"clouds.altitude", {false, SET_FLOAT(t.clouds->altitude)}},
        {"clouds.coverage", {false, SET_FLOAT(t.clouds->coverage)}},
        {"clouds.density", {false, SET_FLOAT(t.clouds->density)}},
        {"clouds.speed", {false, SET_FLOAT(t.clouds->speed)}},
        {"clouds.scale", {false, SET_FLOAT(t.clouds->scale)}},
        {"clouds.thickness", {false, SET_FLOAT(t.clouds->thickness)}},
        {"clouds.viewSteps", {false, SET_INT(t.clouds->viewSteps)}},
        {"clouds.shadowStrength", {false, SET_FLOAT(t.clouds->shadowStrength)}},
        {"clouds.color", {false, SET_VEC3(t.clouds->color)}},
        {"surface.rock.colorA", {false, SURF_VEC3(t.surface->rock.colorA)}},
        {"surface.rock.colorB", {false, SURF_VEC3(t.surface->rock.colorB)}},
        {"surface.rock.roughnessA", {false, SURF_FLOAT(t.surface->rock.roughnessA)}},
        {"surface.rock.roughnessB", {false, SURF_FLOAT(t.surface->rock.roughnessB)}},
        {"surface.rock.streakColor", {false, SURF_VEC3(t.surface->rock.streakColor)}},
        {"surface.rock.streakAmount", {false, SURF_FLOAT(t.surface->rock.streakAmount)}},
        {"surface.rock.tint", {false, SURF_VEC3(t.surface->rock.tint)}},
        {"surface.rock.slope", {false, SURF_FLOAT(t.surface->rock.slope)}},
        {"surface.rock.slopeNoise", {false, SURF_FLOAT(t.surface->rock.slopeNoise)}},
        {"surface.rock.softness", {false, SURF_FLOAT(t.surface->rock.softness)}},
        {"surface.sand.colorA", {false, SURF_VEC3(t.surface->sand.colorA)}},
        {"surface.sand.colorB", {false, SURF_VEC3(t.surface->sand.colorB)}},
        {"surface.sand.roughness", {false, SURF_FLOAT(t.surface->sand.roughness)}},
        {"surface.sand.tint", {false, SURF_VEC3(t.surface->sand.tint)}},
        {"surface.sand.cover", {false, SURF_FLOAT(t.surface->sand.cover)}},
        {"surface.sand.aridity.x", {false, SURF_FLOAT(t.surface->sand.aridity.x)}},
        {"surface.sand.aridity.y", {false, SURF_FLOAT(t.surface->sand.aridity.y)}},
        {"surface.sand.beachHeight.x", {false, SURF_FLOAT(t.surface->sand.beachHeight.x)}},
        {"surface.sand.beachHeight.y", {false, SURF_FLOAT(t.surface->sand.beachHeight.y)}},
        {"surface.sand.desertAridity.x", {false, SURF_FLOAT(t.surface->sand.desertAridity.x)}},
        {"surface.sand.desertAridity.y", {false, SURF_FLOAT(t.surface->sand.desertAridity.y)}},
        {"surface.snow.colorA", {false, SURF_VEC3(t.surface->snow.colorA)}},
        {"surface.snow.colorB", {false, SURF_VEC3(t.surface->snow.colorB)}},
        {"surface.snow.roughness", {false, SURF_FLOAT(t.surface->snow.roughness)}},
        {"surface.snow.tint", {false, SURF_VEC3(t.surface->snow.tint)}},
        {"surface.snow.coldLowering", {false, SURF_FLOAT(t.surface->snow.coldLowering)}},
        {"surface.snow.blendBelow", {false, SURF_FLOAT(t.surface->snow.blendBelow)}},
        {"surface.snow.blendAbove", {false, SURF_FLOAT(t.surface->snow.blendAbove)}},
        {"surface.snow.polarTemperature", {false, SURF_FLOAT(t.surface->snow.polarTemperature)}},
        {"surface.snow.polarAmount", {false, SURF_FLOAT(t.surface->snow.polarAmount)}},
        {"surface.snow.slopeShed.x", {false, SURF_FLOAT(t.surface->snow.slopeShed.x)}},
        {"surface.snow.slopeShed.y", {false, SURF_FLOAT(t.surface->snow.slopeShed.y)}},
        {"surface.snow.shedAmount", {false, SURF_FLOAT(t.surface->snow.shedAmount)}},
        {"surface.ground.lightness.x", {false, SURF_FLOAT(t.surface->ground.lightness.x)}},
        {"surface.ground.lightness.y", {false, SURF_FLOAT(t.surface->ground.lightness.y)}},
        {"surface.ground.roughness", {false, SURF_FLOAT(t.surface->ground.roughness)}},
        {"surface.ground.vegetation", {false, SURF_FLOAT(t.surface->ground.vegetation)}},
        {"surface.ground.soilDryness.x", {false, SURF_FLOAT(t.surface->ground.soilDryness.x)}},
        {"surface.ground.soilDryness.y", {false, SURF_FLOAT(t.surface->ground.soilDryness.y)}},
        {"surface.ground.soilAmount", {false, SURF_FLOAT(t.surface->ground.soilAmount)}},
        {"surface.ground.polarSoil", {false, SURF_FLOAT(t.surface->ground.polarSoil)}},
        {"surface.detail.grain.x", {false, SURF_FLOAT(t.surface->detail.grain.x)}},
        {"surface.detail.grain.y", {false, SURF_FLOAT(t.surface->detail.grain.y)}},
        {"surface.detail.grainSnowFade", {false, SURF_FLOAT(t.surface->detail.grainSnowFade)}},
        {"surface.detail.mediumFrequency", {false, SURF_FLOAT(t.surface->detail.mediumFrequency)}},
        {"surface.detail.fineFrequency", {false, SURF_FLOAT(t.surface->detail.fineFrequency)}},
        {"surface.detail.bumpFrequency", {false, SURF_FLOAT(t.surface->detail.bumpFrequency)}},
        {"surface.detail.bumpTilt", {false, SURF_FLOAT(t.surface->detail.bumpTilt)}},
        {"surface.detail.bumpSnowFade", {false, SURF_FLOAT(t.surface->detail.bumpSnowFade)}},
        {"surface.detail.textureBump", {false, SURF_FLOAT(t.surface->detail.textureBump)}},
        {"surface.detail.roughnessFloor", {false, SURF_FLOAT(t.surface->detail.roughnessFloor)}},
        {"surface.detail.matteRoughnessFloor", {false, SURF_FLOAT(t.surface->detail.matteRoughnessFloor)}},
        {"surface.detail.gamma", {false, SURF_FLOAT(t.surface->detail.gamma)}},
        {"surface.detail.temperatureNoise", {false, SURF_FLOAT(t.surface->detail.temperatureNoise)}},
        {"surface.detail.moistureNoise", {false, SURF_FLOAT(t.surface->detail.moistureNoise)}},
        {"surface.detail.moistureDetailNoise", {false, SURF_FLOAT(t.surface->detail.moistureDetailNoise)}},
        {"surface.seafloor.shallowColor", {false, SURF_VEC3(t.surface->seafloor.shallowColor)}},
        {"surface.seafloor.deepColor", {false, SURF_VEC3(t.surface->seafloor.deepColor)}},
        {"surface.seafloor.depthRange", {false, SURF_FLOAT(t.surface->seafloor.depthRange)}},
        {"surface.climate.polarCaps", {false, SURF_FLOAT(t.surface->climate.polarCaps)}},
        {"surface.climate.substellar", {false, SURF_FLOAT(t.surface->climate.substellar)}},
        {"surface.climate.cityCoverage", {false, SURF_FLOAT(t.surface->climate.cityCoverage)}},
        {"surface.vegetation.strength", {false, SURF_FLOAT(t.surface->vegetation.strength)}},
        {"surface.vegetation.slope.x", {false, SURF_FLOAT(t.surface->vegetation.slope.x)}},
        {"surface.vegetation.slope.y", {false, SURF_FLOAT(t.surface->vegetation.slope.y)}},
        {"surface.vegetation.treeline.x", {false, SURF_FLOAT(t.surface->vegetation.treeline.x)}},
        {"surface.vegetation.treeline.y", {false, SURF_FLOAT(t.surface->vegetation.treeline.y)}},
        {"surface.vegetation.bareColor", {false, SURF_VEC3(t.surface->vegetation.bareColor)}},
        {"surface.sediment.strength", {false, SURF_FLOAT(t.surface->sediment.strength)}},
        {"surface.sediment.color", {false, SURF_VEC3(t.surface->sediment.color)}},
        {"surface.sediment.height.x", {false, SURF_FLOAT(t.surface->sediment.height.x)}},
        {"surface.sediment.height.y", {false, SURF_FLOAT(t.surface->sediment.height.y)}},
        {"surface.sediment.slope.x", {false, SURF_FLOAT(t.surface->sediment.slope.x)}},
        {"surface.sediment.slope.y", {false, SURF_FLOAT(t.surface->sediment.slope.y)}},
        {"surface.sediment.patchiness", {false, SURF_FLOAT(t.surface->sediment.patchiness)}},
        {"surface.wetness.strength", {false, SURF_FLOAT(t.surface->wetness.strength)}},
        {"surface.wetness.shoreWidth", {false, SURF_FLOAT(t.surface->wetness.shoreWidth)}},
        {"surface.wetness.moisture", {false, SURF_FLOAT(t.surface->wetness.moisture)}},
        {"surface.wetness.darkening", {false, SURF_FLOAT(t.surface->wetness.darkening)}},
        {"surface.wetness.roughness", {false, SURF_FLOAT(t.surface->wetness.roughness)}},
        {"surface.wetness.tint", {false, SURF_VEC3(t.surface->wetness.tint)}},
        {"surface.strata.strength", {false, SURF_FLOAT(t.surface->strata.strength)}},
        {"surface.strata.bands", {false, SURF_FLOAT(t.surface->strata.bands)}},
        {"surface.strata.warp", {false, SURF_FLOAT(t.surface->strata.warp)}},
        {"surface.strata.colorA", {false, SURF_VEC3(t.surface->strata.colorA)}},
        {"surface.strata.colorB", {false, SURF_VEC3(t.surface->strata.colorB)}},
        {"surface.textures.rock.fineTile", {false, SURF_FLOAT(t.surface->textures[0].fineTile)}},
        {"surface.textures.rock.coarseTile", {false, SURF_FLOAT(t.surface->textures[0].coarseTile)}},
        {"surface.textures.rock.strength", {false, SURF_FLOAT(t.surface->textures[0].strength)}},
        {"surface.textures.rock.normalStrength", {false, SURF_FLOAT(t.surface->textures[0].normalStrength)}},
        {"surface.textures.grass.fineTile", {false, SURF_FLOAT(t.surface->textures[1].fineTile)}},
        {"surface.textures.grass.coarseTile", {false, SURF_FLOAT(t.surface->textures[1].coarseTile)}},
        {"surface.textures.grass.strength", {false, SURF_FLOAT(t.surface->textures[1].strength)}},
        {"surface.textures.grass.normalStrength", {false, SURF_FLOAT(t.surface->textures[1].normalStrength)}},
        {"surface.textures.sand.fineTile", {false, SURF_FLOAT(t.surface->textures[2].fineTile)}},
        {"surface.textures.sand.coarseTile", {false, SURF_FLOAT(t.surface->textures[2].coarseTile)}},
        {"surface.textures.sand.strength", {false, SURF_FLOAT(t.surface->textures[2].strength)}},
        {"surface.textures.sand.normalStrength", {false, SURF_FLOAT(t.surface->textures[2].normalStrength)}},
        {"surface.textures.snow.fineTile", {false, SURF_FLOAT(t.surface->textures[3].fineTile)}},
        {"surface.textures.snow.coarseTile", {false, SURF_FLOAT(t.surface->textures[3].coarseTile)}},
        {"surface.textures.snow.strength", {false, SURF_FLOAT(t.surface->textures[3].strength)}},
        {"surface.textures.snow.normalStrength", {false, SURF_FLOAT(t.surface->textures[3].normalStrength)}},
        {"surface.textures.soil.fineTile", {false, SURF_FLOAT(t.surface->textures[4].fineTile)}},
        {"surface.textures.soil.coarseTile", {false, SURF_FLOAT(t.surface->textures[4].coarseTile)}},
        {"surface.textures.soil.strength", {false, SURF_FLOAT(t.surface->textures[4].strength)}},
        {"surface.textures.soil.normalStrength", {false, SURF_FLOAT(t.surface->textures[4].normalStrength)}},

        // ---- sea level (real-time) ----
        {"seaLevel",
         {false,
          [](const ParamTargets& t, const ParamValue& v) -> bool
          {
              if (!t.seaLevel)
                  return false;
              float f;
              if (!AsFloat(v, f))
                  return false;
              *t.seaLevel = f;
              return true;
          }}},

        // ---- LOD (needs regen) ----
        {"lod.enabled", {true, SET_BOOL(t.lod->enabled)}},
        {"lod.patchSubdivisions", {true, SET_INT(t.lod->patchSubdivisions)}},
        {"lod.meshResolution", {true, SET_INT(t.lod->meshResolution)}},
        {"lod.maxDepth", {true, SET_INT(t.lod->maxDepth)}},
        {"lod.splitThreshold", {true, SET_FLOAT(t.lod->splitThreshold)}},
        {"lod.hysteresis", {true, SET_FLOAT(t.lod->hysteresis)}},
        {"lod.maxActivePatches", {true, SET_INT(t.lod->maxActivePatches)}},
        {"lod.skirtFraction", {true, SET_FLOAT(t.lod->skirtFraction)}},

        // ---- generation (needs regen) ----
        {"gen.subdivisions", {true, SET_INT(t.gen->subdivisions)}},

        // ---- body.shading.* (real-time: pushed per-frame by SetRenderUniforms) ----
        {"body.shading.biomesEnabled", {false, BODY_BOOL(t.body->shading.biomesEnabled)}},
        {"body.shading.steepnessThreshold", {false, BODY_FLOAT(t.body->shading.steepnessThreshold)}},
        {"body.shading.flatToSteepBlend", {false, BODY_FLOAT(t.body->shading.flatToSteepBlend)}},
        {"body.shading.snowLatitude", {false, BODY_FLOAT(t.body->shading.snowLatitude)}},
        {"body.shading.snowBlend", {false, BODY_FLOAT(t.body->shading.snowBlend)}},
        {"body.shading.snowLine", {false, BODY_FLOAT(t.body->shading.snowLine)}},
        {"body.shading.shoreHeight", {false, BODY_FLOAT(t.body->shading.shoreHeight)}},
        {"body.shading.aoStrength", {false, BODY_FLOAT(t.body->shading.aoStrength)}},
        {"body.shading.colorSnow", {false, BODY_VEC3(t.body->shading.colorSnow)}},
        {"body.shading.colorShoreLow", {false, BODY_VEC3(t.body->shading.colorShoreLow)}},
        {"body.shading.colorSteepLow", {false, BODY_VEC3(t.body->shading.colorSteepLow)}},

        // ---- body.metadata.radius (needs regen; the LOD reads it from the body) ----
        {"body.metadata.radius", {true, BODY_FLOAT(t.body->metadata.radius)}},

        // ---- body.shape.* (needs regen) ----
        {"body.shape.heightScale", {true, BODY_FLOAT(t.body->shape.heightScale)}},
        {"body.shape.normalEpsilon", {true, BODY_FLOAT(t.body->shape.normalEpsilon)}},
        {"body.shape.oceanDepthMultiplier", {true, BODY_FLOAT(t.body->shape.oceanDepthMultiplier)}},
        {"body.shape.mountainBlend", {true, BODY_FLOAT(t.body->shape.mountainBlend)}},
        {"body.shape.continentBaseLevel", {true, BODY_FLOAT(t.body->shape.continentBaseLevel)}},
        {"body.shape.globalFrequency", {true, BODY_FLOAT(t.body->shape.globalFrequency)}},
        {"body.shape.mountainPower", {true, BODY_FLOAT(t.body->shape.mountainPower)}},
        {"body.shape.continentCount", {true, BODY_INT(t.body->shape.continentCount)}},
        {"body.shape.continentClustering", {true, BODY_FLOAT(t.body->shape.continentClustering)}},

        // ---- body.tectonics.* (needs regen) ----
        {"body.tectonics.enabled", {true, BODY_BOOL(t.body->tectonics.enabled)}},
        {"body.tectonics.numPlates", {true, BODY_INT(t.body->tectonics.numPlates)}},
        {"body.tectonics.continentalFraction", {true, BODY_FLOAT(t.body->tectonics.continentalFraction)}},
        {"body.tectonics.convergentMountainScale", {true, BODY_FLOAT(t.body->tectonics.convergentMountainScale)}},

        // ---- body.oceanFloor.* (needs regen) ----
        {"body.oceanFloor.trenchDepth", {true, BODY_FLOAT(t.body->oceanFloor.trenchDepth)}},
        {"body.oceanFloor.oceanRidgeStrength", {true, BODY_FLOAT(t.body->oceanFloor.oceanRidgeStrength)}},
        {"body.oceanFloor.shelfWidth", {true, BODY_FLOAT(t.body->oceanFloor.shelfWidth)}},

        // ---- body.heightDetail.* (needs regen) ----
        {"body.heightDetail.perturbScale", {true, BODY_FLOAT(t.body->heightDetail.perturbScale)}},
        {"body.heightDetail.perturbStrengthHigh", {true, BODY_FLOAT(t.body->heightDetail.perturbStrengthHigh)}},

        // ---- body.erosion.* (needs regen) ----
        {"body.erosion.enabled", {true, BODY_BOOL(t.body->erosion.enabled)}},
        {"body.erosion.iterations", {true, BODY_INT(t.body->erosion.iterations)}},

        // ---- body.macroWorld.* (needs regen; re-bakes the macro field) ----
        // ---- body.terrainStyle.* (needs regen) ----
        {"body.terrainStyle.dunes", {true, BODY_FLOAT(t.body->terrainStyle.dunes)}},
        {"body.terrainStyle.duneScale", {true, BODY_FLOAT(t.body->terrainStyle.duneScale)}},
        {"body.terrainStyle.craters", {true, BODY_FLOAT(t.body->terrainStyle.craters)}},
        {"body.terrainStyle.craterScale", {true, BODY_FLOAT(t.body->terrainStyle.craterScale)}},
        {"body.terrainStyle.terraces", {true, BODY_FLOAT(t.body->terrainStyle.terraces)}},
        {"body.terrainStyle.terraceSteps", {true, BODY_FLOAT(t.body->terrainStyle.terraceSteps)}},
        {"body.terrainStyle.canyons", {true, BODY_FLOAT(t.body->terrainStyle.canyons)}},
        {"body.terrainStyle.spires", {true, BODY_FLOAT(t.body->terrainStyle.spires)}},
        {"body.macroWorld.enabled", {true, BODY_BOOL(t.body->macroWorld.enabled)}},
        {"body.macroWorld.resolution", {true, BODY_INT(t.body->macroWorld.resolution)}},
        {"body.macroWorld.convergentThreshold", {true, BODY_FLOAT(t.body->macroWorld.convergentThreshold)}},
        {"body.macroWorld.erosionIterations", {true, BODY_INT(t.body->macroWorld.erosionIterations)}},
        {"body.macroWorld.rainRate", {true, BODY_FLOAT(t.body->macroWorld.rainRate)}},
        {"body.macroWorld.erosionRate", {true, BODY_FLOAT(t.body->macroWorld.erosionRate)}},
        {"body.macroWorld.depositionRate", {true, BODY_FLOAT(t.body->macroWorld.depositionRate)}},
        {"body.macroWorld.moistureIterations", {true, BODY_INT(t.body->macroWorld.moistureIterations)}},
        {"body.macroWorld.windStrength", {true, BODY_FLOAT(t.body->macroWorld.windStrength)}},
        {"body.macroWorld.landDrying", {true, BODY_FLOAT(t.body->macroWorld.landDrying)}},
        {"body.macroWorld.orographicStrength", {true, BODY_FLOAT(t.body->macroWorld.orographicStrength)}},
    };
    return table;
}

} // namespace

ApplyResult ApplyParam(const ParamTargets& targets, const std::string& key, const ParamValue& value)
{
    const auto& table = Table();
    auto it = table.find(key);
    if (it == table.end())
        return {false, false};

    const bool ok = it->second.set(targets, value);
    return {ok, ok && it->second.needsRegen};
}

bool IsKnownParam(const std::string& key)
{
    const auto& table = Table();
    return table.find(key) != table.end();
}

} // namespace planets::app::instrument
