#pragma once

#include "SurfaceSettings.h"
#include <glm/glm.hpp>

namespace planets::render
{

// std140 layout of SurfaceBlock in shaders/includes/surface_params.glsl: vec4 rows only, so C++ and
// GLSL agree without padding rules. Row meanings are commented in the GLSL block.
struct SurfaceBlock
{
    glm::vec4 rockColorA, rockColorB, rockStreak, rockTint;
    glm::vec4 sandColorA, sandColorB, sandTint;
    glm::vec4 snowColorA, snowColorB, snowTint;
    glm::vec4 ground, rockRule, snowRule0, snowRule1, sandRule, soilRule, soilRule2;
    glm::vec4 detail0, detail1, detail2, detail3;
    glm::vec4 seafloorShallow, seafloorDeep;
    glm::vec4 climate;
    glm::vec4 texture[SurfaceSettings::TextureCount];
    glm::vec4 wet0, wet1;
    glm::vec4 strata0, strata1, strata2;
};

static_assert(sizeof(SurfaceBlock) == 34 * 16, "SurfaceBlock must match the std140 GLSL block");

// Uniform buffer binding point of SurfaceBlock (separate namespace from the SSBO bindings).
inline constexpr unsigned int SurfaceBlockBindingPoint = 5;

inline SurfaceBlock PackSurfaceBlock(const SurfaceSettings& s)
{
    SurfaceBlock b{};
    b.rockColorA = glm::vec4(s.rock.colorA, s.rock.roughnessA);
    b.rockColorB = glm::vec4(s.rock.colorB, s.rock.roughnessB);
    b.rockStreak = glm::vec4(s.rock.streakColor, s.rock.streakAmount);
    b.rockTint = glm::vec4(s.rock.tint, 0.0f);
    b.sandColorA = glm::vec4(s.sand.colorA, s.sand.roughness);
    b.sandColorB = glm::vec4(s.sand.colorB, 0.0f);
    b.sandTint = glm::vec4(s.sand.tint, s.sand.cover);
    b.snowColorA = glm::vec4(s.snow.colorA, s.snow.roughness);
    b.snowColorB = glm::vec4(s.snow.colorB, 0.0f);
    b.snowTint = glm::vec4(s.snow.tint, 0.0f);
    b.ground = glm::vec4(s.ground.lightness, s.ground.roughness, s.ground.vegetation);
    b.rockRule = glm::vec4(s.rock.slope, s.rock.slopeNoise, s.rock.softness, 0.0f);
    b.snowRule0 = glm::vec4(s.snow.coldLowering, s.snow.blendBelow, s.snow.blendAbove, s.snow.polarTemperature);
    b.snowRule1 = glm::vec4(s.snow.polarAmount, s.snow.slopeShed, s.snow.shedAmount);
    b.sandRule = glm::vec4(s.sand.aridity, s.sand.beachHeight);
    b.soilRule = glm::vec4(s.sand.desertAridity, s.ground.soilDryness);
    b.soilRule2 = glm::vec4(s.ground.soilAmount, s.ground.polarSoil, 0.0f, 0.0f);
    b.detail0 = glm::vec4(s.detail.grain, s.detail.grainSnowFade, s.detail.mediumFrequency);
    b.detail1 = glm::vec4(s.detail.bumpFrequency, s.detail.bumpTilt, s.detail.bumpSnowFade, s.detail.textureBump);
    b.detail2 = glm::vec4(s.detail.roughnessFloor, s.detail.matteRoughnessFloor, s.detail.gamma,
                          s.detail.fineFrequency);
    b.detail3 = glm::vec4(s.detail.temperatureNoise, s.detail.moistureNoise, s.detail.moistureDetailNoise, 0.0f);
    b.seafloorShallow = glm::vec4(s.seafloor.shallowColor, s.seafloor.depthRange);
    b.seafloorDeep = glm::vec4(s.seafloor.deepColor, 0.0f);
    b.climate = glm::vec4(s.climate.polarCaps, s.climate.substellar, s.climate.cityCoverage, 0.0f);
    for (int i = 0; i < SurfaceSettings::TextureCount; ++i)
    {
        const auto& t = s.textures[i];
        b.texture[i] = glm::vec4(t.fineTile, t.coarseTile, t.strength, t.normalStrength);
    }
    b.strata0 = glm::vec4(s.strata.colorA, s.strata.strength);
    b.strata1 = glm::vec4(s.strata.colorB, s.strata.bands);
    b.strata2 = glm::vec4(s.strata.warp, 0.0f, 0.0f, 0.0f);
    b.wet0 = glm::vec4(s.wetness.tint, s.wetness.strength);
    b.wet1 = glm::vec4(s.wetness.shoreWidth, s.wetness.moisture, s.wetness.darkening, s.wetness.roughness);
    return b;
}

} // namespace planets::render
