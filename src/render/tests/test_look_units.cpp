#include <catch2/catch_test_macros.hpp>

#include "types/LookUnits.h"

#include "planetgen/model/BodyConfigJson.h"

#include <filesystem>
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

TEST_CASE("look_units: a roll over the default scene drops edited exposure unless Light is kept", "[look_units]")
{
    const auto body = LoadBody("data/bodies/lava.json");
    const SceneSettings defaults;
    SceneSettings edited;
    edited.exposure = 2.5f;
    edited.lightDir = glm::normalize(glm::vec3(0.3f, 0.8f, 0.1f));

    LookState fromDefault;
    LookState fromEdited;
    REQUIRE(RollLook(body, 5, defaults, fromDefault).empty());
    REQUIRE(RollLook(body, 5, edited, fromEdited).empty());
    CHECK(fromDefault.scene.exposure == defaults.exposure);
    CHECK(fromEdited.scene.exposure == 2.5f);
    CHECK(fromDefault.scene.lightDir == defaults.lightDir);

    // The live world carries the edit and its baseline does not.
    LookState live = fromDefault;
    live.scene = edited;

    const MergedRoll open = MergeRoll(fromDefault, live, fromDefault, AllUnits & ~UnitBit(LookUnit::Light));
    CHECK(open.next.scene.exposure == defaults.exposure);
    CHECK(open.baseline.scene.exposure == defaults.exposure);

    const MergedRoll locked = MergeRoll(fromDefault, live, fromDefault, UnitBit(LookUnit::Light));
    CHECK(locked.next.scene.exposure == 2.5f);
    CHECK(locked.next.scene.lightDir == edited.lightDir);
    CHECK(locked.baseline.scene.exposure == defaults.exposure);
    // Other units still take the roll.
    CHECK(locked.next.clouds.coverage == fromDefault.clouds.coverage);
}

namespace
{
std::vector<std::filesystem::path> ShippedBodies()
{
    std::vector<std::filesystem::path> out;
    for (const auto& e : std::filesystem::directory_iterator("data/bodies"))
        if (e.path().extension() == ".json")
            out.push_back(e.path());
    return out;
}

// "render.ocean.crust" -> the nested value in a config JSON; null when absent.
nlohmann::json ValueAt(const nlohmann::json& root, const std::string& key)
{
    const nlohmann::json* node = &root;
    size_t start = 0;
    while (start <= key.size())
    {
        const size_t dot = key.find('.', start);
        const std::string part = key.substr(start, dot == std::string::npos ? std::string::npos : dot - start);
        if (!node->is_object() || !node->contains(part))
            return nullptr;
        node = &(*node)[part];
        if (dot == std::string::npos)
            break;
        start = dot + 1;
    }
    return *node;
}
} // namespace

TEST_CASE("look_units: copying one unit gives that unit of the source, for every shipped body", "[look_units]")
{
    const auto bodies = ShippedBodies();
    REQUIRE(!bodies.empty());
    for (const auto& path : bodies)
    {
        const auto body = LoadBody(path.string().c_str());
        const LookState a = Roll(body, 11);
        const LookState b = Roll(body, 22);
        for (uint32_t u = 0; u < static_cast<uint32_t>(LookUnit::Count); ++u)
        {
            const auto unit = static_cast<LookUnit>(u);
            LookState next = b;
            CopyUnits(UnitBit(unit), a, next);
            INFO(path.string() << " unit " << u);
            CHECK(UnitsEqual(unit, next, a));
        }
    }
}

TEST_CASE("look_units: every config variety key survives a lock on its unit", "[look_units]")
{
    for (const auto& path : ShippedBodies())
    {
        const auto body = LoadBody(path.string().c_str());
        if (!body.contains("variety"))
            continue;
        const LookState a = Roll(body, 11);
        const LookState b = Roll(body, 22);
        const nlohmann::json ja = nlohmann::json::parse(planetgen::BodyConfigToJson(a.config));
        for (const auto& item : body["variety"].items())
        {
            const std::string& key = item.key();
            // Look keys (render.*) are covered by the unit comparison above.
            if (key.rfind("render.", 0) == 0)
                continue;
            LookState next = b;
            CopyUnits(UnitBit(UnitOfKey(key)), a, next);
            const nlohmann::json jn = nlohmann::json::parse(planetgen::BodyConfigToJson(next.config));
            INFO(path.string() << " " << key);
            CHECK(ValueAt(jn, key) == ValueAt(ja, key));
        }
    }
}

TEST_CASE("look_units: Terrain copies the metadata a variety key can hit", "[look_units]")
{
    CHECK(UnitOfKey("metadata.atmosphereHeight") == LookUnit::Terrain);
    LookState from;
    from.config.metadata.atmosphereHeight = 0.2f;
    from.config.metadata.seaLevel = 1.05f;
    LookState to;
    CopyUnits(UnitBit(LookUnit::Terrain), from, to);
    CHECK(to.config.metadata.atmosphereHeight == 0.2f);
    CHECK(to.config.metadata.seaLevel == planetgen::MetadataBlock{}.seaLevel);
}

TEST_CASE("look_units: a merged roll keeps the kept units live and their default old", "[look_units]")
{
    const TwoWorlds w;
    LookState live = w.a;
    live.ocean.crust = 0.77f; // edited
    LookState oldBaseline = w.a;
    const MergedRoll m = MergeRoll(w.b, live, oldBaseline, UnitBit(LookUnit::Ocean));
    CHECK(m.next.ocean.crust == 0.77f);
    CHECK(m.baseline.ocean.crust == w.a.ocean.crust);
    CHECK(m.next.clouds.coverage == w.b.clouds.coverage);
    CHECK(m.baseline.clouds.coverage == w.b.clouds.coverage);
    // No kept unit: the roll is both the world and its baseline.
    const MergedRoll fresh = MergeRoll(w.b, live, oldBaseline, 0);
    CHECK(fresh.next.ocean.crust == w.b.ocean.crust);
    CHECK(fresh.baseline.ocean.crust == w.b.ocean.crust);
}

TEST_CASE("look_units: ChangedUnits flags only the tab whose value moved", "[look_units]")
{
    const TwoWorlds w;
    CHECK(ChangedUnits(w.a, w.a) == 0u);

    LookState live = w.a;
    live.scene.lighting.ambientLight += 0.1f;
    CHECK(ChangedUnits(live, w.a) == UnitBit(LookUnit::Light));

    live = w.a;
    live.atmosphere.enabled = !live.atmosphere.enabled;
    CHECK(ChangedUnits(live, w.a) == UnitBit(LookUnit::Sky));

    live = w.a;
    live.config.metadata.seaLevel += 0.01f;
    live.surface.seafloor.depthRange += 0.1f;
    CHECK(ChangedUnits(live, w.a) == UnitBit(LookUnit::Ocean));

    live = w.a;
    live.surface.rock.slope += 0.1f;
    live.config.terrainStyle.spires += 0.1f;
    CHECK(ChangedUnits(live, w.a) == (UnitBit(LookUnit::Surface) | UnitBit(LookUnit::Terrain)));
}
