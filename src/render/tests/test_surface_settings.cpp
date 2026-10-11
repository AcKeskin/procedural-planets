#include <catch2/catch_test_macros.hpp>

#include "settings/SurfaceBlock.h"
#include "types/BodyLook.h"
#include "types/BodyVariety.h"

#include <nlohmann/json.hpp>

using namespace planets::render;

TEST_CASE("surface: no render.surface block keeps today's shader constants", "[surface]")
{
    const SurfaceSettings s = types::ParseBodyLook(nlohmann::json::object()).surface;
    CHECK(s.rock.slope == 0.35f);
    CHECK(s.snow.slopeShed == glm::vec2(0.55f, 0.85f));
    CHECK(s.detail.bumpTilt == 0.012f);
    CHECK(s.textures[2].fineTile == 0.07f);
    CHECK(s.seafloor.depthRange == 0.08f);
}

TEST_CASE("surface: render.surface fields override the defaults by group", "[surface]")
{
    const auto look = types::ParseBodyLook(nlohmann::json::parse(R"({"render": {"surface": {
        "rock": {"slope": 0.2, "colorA": [0.1, 0.2, 0.3]},
        "snow": {"slopeShed": [0.4, 0.9]},
        "detail": {"gamma": 1.0},
        "textures": {"sand": {"fineTile": 0.2, "normalStrength": 2.0}},
        "climate": {"polarCaps": 0.3}}}})"));
    const SurfaceSettings& s = look.surface;
    CHECK(s.rock.slope == 0.2f);
    CHECK(s.rock.colorA == glm::vec3(0.1f, 0.2f, 0.3f));
    CHECK(s.rock.softness == SurfaceSettings{}.rock.softness);
    CHECK(s.snow.slopeShed == glm::vec2(0.4f, 0.9f));
    CHECK(s.detail.gamma == 1.0f);
    CHECK(s.textures[2].fineTile == 0.2f);
    CHECK(s.textures[2].normalStrength == 2.0f);
    CHECK(s.textures[0].fineTile == SurfaceSettings{}.textures[0].fineTile);
    CHECK(s.climate.polarCaps == 0.3f);
}

TEST_CASE("surface: older materials keys still apply; render.surface wins over them", "[surface]")
{
    const auto look = types::ParseBodyLook(nlohmann::json::parse(R"({"render": {
        "materials": {"rockTint": [0.5, 0.5, 0.5], "vegetation": 0.0, "sandCover": 1.0},
        "surface": {"rock": {"tint": [0.2, 0.2, 0.2]}}}})"));
    CHECK(look.surface.rock.tint == glm::vec3(0.2f));
    CHECK(look.surface.ground.vegetation == 0.0f);
    CHECK(look.surface.sand.cover == 1.0f);
}

TEST_CASE("surface: a variety rule on render.surface rolls a value inside its range", "[surface]")
{
    const auto body = nlohmann::json::parse(R"({"render": {"surface": {"rock": {"slope": 0.35}}},
        "variety": {"render.surface.rock.slope": {"range": [0.1, 0.2]}}})");
    for (uint32_t seed = 0; seed < 20; ++seed)
    {
        const float slope = types::ParseBodyLook(types::RollVariety(body, seed)).surface.rock.slope;
        CHECK(slope >= 0.1f);
        CHECK(slope <= 0.2f);
    }
}

TEST_CASE("surface: strata is off by default and read from render.surface.strata", "[surface]")
{
    CHECK(SurfaceSettings{}.strata.strength == 0.0f);
    const auto look = types::ParseBodyLook(nlohmann::json::parse(R"({"render": {"surface": {
        "strata": {"strength": 0.8}}}})"));
    CHECK(look.surface.strata.strength == 0.8f);
    CHECK(PackSurfaceBlock(look.surface).strata0.w == 0.8f);
}

TEST_CASE("surface: wetness is off by default and read from render.surface.wetness", "[surface]")
{
    CHECK(SurfaceSettings{}.wetness.strength == 0.0f);
    const auto look = types::ParseBodyLook(nlohmann::json::parse(R"({"render": {"surface": {
        "wetness": {"strength": 0.6}}}})"));
    CHECK(look.surface.wetness.strength == 0.6f);
    CHECK(PackSurfaceBlock(look.surface).wet0.w == 0.6f);
}

TEST_CASE("surface: sediment is off by default and read from render.surface.sediment", "[surface]")
{
    CHECK(SurfaceSettings{}.sediment.strength == 0.0f);
    const auto look = types::ParseBodyLook(nlohmann::json::parse(R"({"render": {"surface": {
        "sediment": {"strength": 0.7}}}})"));
    CHECK(look.surface.sediment.strength == 0.7f);
    CHECK(PackSurfaceBlock(look.surface).sed0.w == 0.7f);
}

TEST_CASE("surface: vegetation is off by default and read from render.surface.vegetation", "[surface]")
{
    CHECK(SurfaceSettings{}.vegetation.strength == 0.0f);
    const auto look = types::ParseBodyLook(nlohmann::json::parse(R"({"render": {"surface": {
        "vegetation": {"strength": 0.9}}}})"));
    CHECK(look.surface.vegetation.strength == 0.9f);
    CHECK(PackSurfaceBlock(look.surface).veg0.w == 0.9f);
}

TEST_CASE("surface: cover is off by default and read from render.surface.cover", "[surface]")
{
    CHECK(SurfaceSettings{}.cover.strength == 0.0f);
    const auto look = types::ParseBodyLook(nlohmann::json::parse(R"({"render": {"surface": {
        "cover": {"strength": 0.5}}}})"));
    CHECK(look.surface.cover.strength == 0.5f);
    CHECK(PackSurfaceBlock(look.surface).cover0.w == 0.5f);
}

TEST_CASE("surface: skyAmbient is off by default and read from render.surface.skyAmbient", "[surface]")
{
    CHECK(SurfaceSettings{}.skyAmbient.strength == 0.0f);
    const auto look = types::ParseBodyLook(nlohmann::json::parse(R"({"render": {"surface": {
        "skyAmbient": {"strength": 0.6}}}})"));
    CHECK(look.surface.skyAmbient.strength == 0.6f);
    CHECK(PackSurfaceBlock(look.surface).amb0.w == 0.6f);
}

TEST_CASE("surface: wrong types are ignored", "[surface]")
{
    const auto look = types::ParseBodyLook(nlohmann::json::parse(R"({"render": {"surface": {
        "rock": {"slope": "steep", "colorA": [1, 2]}, "snow": {"slopeShed": [0.1]}}}})"));
    CHECK(look.surface.rock.slope == SurfaceSettings{}.rock.slope);
    CHECK(look.surface.rock.colorA == SurfaceSettings{}.rock.colorA);
    CHECK(look.surface.snow.slopeShed == SurfaceSettings{}.snow.slopeShed);
}

TEST_CASE("surface: the packed block carries the values to the rows the shader reads", "[surface]")
{
    SurfaceSettings s;
    s.rock.slope = 0.25f;
    s.sand.cover = 0.75f;
    s.textures[4].normalStrength = 3.0f;
    s.climate.cityCoverage = 0.5f;
    const SurfaceBlock b = PackSurfaceBlock(s);
    CHECK(b.rockRule.x == 0.25f);
    CHECK(b.sandTint.w == 0.75f);
    CHECK(b.texture[4].w == 3.0f);
    CHECK(b.climate.z == 0.5f);
    CHECK(b.detail2.z == s.detail.gamma);
}
