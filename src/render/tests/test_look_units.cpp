#include <catch2/catch_test_macros.hpp>

#include "types/LookUnits.h"

#include <fstream>
#include <nlohmann/json.hpp>

using namespace planets::render;
using namespace planets::render::types;

namespace
{
nlohmann::json LoadBody(const char* path)
{
    std::ifstream in(path);
    return nlohmann::json::parse(in);
}

LookState Roll(const nlohmann::json& body, uint32_t seed)
{
    LookState s;
    REQUIRE(RollLook(body, seed, SceneSettings{}, s).empty());
    return s;
}

// Two seeds of the lava type: every unit it rolls differs between them.
struct TwoWorlds
{
    nlohmann::json body = LoadBody("data/bodies/lava.json");
    LookState a = Roll(body, 1);
    LookState b = Roll(body, 2);
};
} // namespace

TEST_CASE("look_units: variety keys map to the tab that shows them", "[look_units]")
{
    CHECK(UnitOfKey("terrainStyle.craters") == LookUnit::Terrain);
    CHECK(UnitOfKey("tectonics.coastlineNoise") == LookUnit::Terrain);
    CHECK(UnitOfKey("render.surface.strata.strength") == LookUnit::Surface);
    CHECK(UnitOfKey("render.palette") == LookUnit::Surface);
    CHECK(UnitOfKey("render.climate.polarCaps") == LookUnit::Surface);
    CHECK(UnitOfKey("metadata.seaLevel") == LookUnit::Ocean);
    CHECK(UnitOfKey("render.ocean.crust") == LookUnit::Ocean);
    CHECK(UnitOfKey("render.surface.seafloor.deepColor") == LookUnit::Ocean);
    CHECK(UnitOfKey("render.clouds.coverage") == LookUnit::Sky);
    CHECK(UnitOfKey("render.weather") == LookUnit::Sky);
    CHECK(UnitOfKey("render.lighting.hazeStrength") == LookUnit::Sky);
    CHECK(UnitOfKey("render.star.class") == LookUnit::Light);
    CHECK(UnitOfKey("render.lighting.nightLights") == LookUnit::Light);
    CHECK(UnitOfKey("render.surface.skyAmbient.strength") == LookUnit::Light);
}

TEST_CASE("look_units: Earth has no random values, lava rolls in every look tab", "[look_units]")
{
    CHECK(VarietyUnits(LoadBody("data/bodies/earth.json")) == 0u);
    CHECK(VarietyUnits(LoadBody("data/bodies/lava.json")) == AllUnits);
}

TEST_CASE("look_units: the same seed rolls the same world", "[look_units]")
{
    const auto body = LoadBody("data/bodies/lava.json");
    const LookState a = Roll(body, 7);
    const LookState b = Roll(body, 7);
    CHECK(a.config.terrainStyle.spires == b.config.terrainStyle.spires);
    CHECK(a.clouds.coverage == b.clouds.coverage);
    CHECK(a.ocean.crust == b.ocean.crust);
    CHECK(a.scene.sunColor == b.scene.sunColor);
    CHECK(a.surface.strata.strength == b.surface.strata.strength);
}

TEST_CASE("look_units: a kept unit survives a reroll, the others take the new seed", "[look_units]")
{
    const TwoWorlds w;
    REQUIRE(w.a.clouds.coverage != w.b.clouds.coverage);
    REQUIRE(w.a.ocean.crust != w.b.ocean.crust);
    REQUIRE(w.a.surface.strata.strength != w.b.surface.strata.strength);

    // Reroll from a to b with Ocean locked: next = b, then the locked unit copied back from a.
    LookState next = w.b;
    CopyUnits(UnitBit(LookUnit::Ocean), w.a, next);
    CHECK(next.ocean.crust == w.a.ocean.crust);
    CHECK(next.ocean.deepColor == w.a.ocean.deepColor);
    CHECK(next.config.metadata.seaLevel == w.a.config.metadata.seaLevel);
    CHECK(next.clouds.coverage == w.b.clouds.coverage);
    CHECK(next.surface.strata.strength == w.b.surface.strata.strength);
    CHECK(next.config.terrainStyle.spires == w.b.config.terrainStyle.spires);
}

TEST_CASE("look_units: shared structs split by tab", "[look_units]")
{
    LookState from;
    from.surface.seafloor.depthRange = 0.2f;
    from.surface.skyAmbient.strength = 0.9f;
    from.surface.rock.slope = 0.7f;
    from.scene.lighting.hazeStrength = 0.33f;
    from.scene.lighting.ambientLight = 0.44f;
    from.config.shading.snowLine = 1.2f;
    from.palette = {glm::vec3(1.0f)};

    LookState to;
    CopyUnits(UnitBit(LookUnit::Surface), from, to);
    CHECK(to.surface.rock.slope == 0.7f);
    CHECK(to.config.shading.snowLine == 1.2f);
    CHECK(to.palette.size() == 1);
    CHECK(to.surface.seafloor.depthRange == SurfaceSettings{}.seafloor.depthRange);
    CHECK(to.surface.skyAmbient.strength == SurfaceSettings{}.skyAmbient.strength);

    LookState ocean;
    CopyUnits(UnitBit(LookUnit::Ocean), from, ocean);
    CHECK(ocean.surface.seafloor.depthRange == 0.2f);
    CHECK(ocean.surface.rock.slope == SurfaceSettings{}.rock.slope);

    LookState light;
    CopyUnits(UnitBit(LookUnit::Light), from, light);
    CHECK(light.surface.skyAmbient.strength == 0.9f);
    CHECK(light.scene.lighting.ambientLight == 0.44f);
    CHECK(light.scene.lighting.hazeStrength == LightingSettings{}.hazeStrength);

    LookState sky;
    CopyUnits(UnitBit(LookUnit::Sky), from, sky);
    CHECK(sky.scene.lighting.hazeStrength == 0.33f);
    CHECK(sky.scene.lighting.ambientLight == LightingSettings{}.ambientLight);
}

TEST_CASE("look_units: copying every unit gives the source world", "[look_units]")
{
    const TwoWorlds w;
    LookState next = w.b;
    CopyUnits(AllUnits, w.a, next);
    CHECK(next.config.terrainStyle.spires == w.a.config.terrainStyle.spires);
    CHECK(next.config.tectonics.coastlineNoise == w.a.config.tectonics.coastlineNoise);
    CHECK(next.clouds.coverage == w.a.clouds.coverage);
    CHECK(next.ocean.crust == w.a.ocean.crust);
    CHECK(next.scene.sunColor == w.a.scene.sunColor);
    CHECK(next.surface.strata.strength == w.a.surface.strata.strength);
    CHECK(next.palette == w.a.palette);
}
