#include "ParamRegistry.h"

#include <model/BodyConfig.h>

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

        // ---- atmosphere (real-time) ----
        {"atmosphere.enabled", {false, SET_BOOL(t.atmosphere->enabled)}},
        {"atmosphere.atmosphereScale", {false, SET_FLOAT(t.atmosphere->atmosphereScale)}},
        {"atmosphere.wavelengths", {false, SET_VEC3(t.atmosphere->wavelengths)}},
        {"atmosphere.scatteringStrength", {false, SET_FLOAT(t.atmosphere->scatteringStrength)}},
        {"atmosphere.densityFalloff", {false, SET_FLOAT(t.atmosphere->densityFalloff)}},
        {"atmosphere.mieCoefficient", {false, SET_FLOAT(t.atmosphere->mieCoefficient)}},
        {"atmosphere.mieDensityFalloff", {false, SET_FLOAT(t.atmosphere->mieDensityFalloff)}},
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
        {"clouds.color", {false, SET_VEC3(t.clouds->color)}},
        {"clouds.shadowStrength", {false, SET_FLOAT(t.clouds->shadowStrength)}},

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
        {"lod.planetRadius", {true, SET_FLOAT(t.lod->planetRadius)}},
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
