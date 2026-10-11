#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "types/BodyLook.h"

#include <nlohmann/json.hpp>

using namespace planets::render;
using Catch::Matchers::WithinAbs;

TEST_CASE("body_look: no render block gives the app defaults", "[body_look]")
{
    const auto look = types::ParseBodyLook(nlohmann::json::parse(R"({"metadata": {"name": "Earth"}})"));
    CHECK(look.atmosphere.scatteringStrength == effects::AtmosphereSettings{}.scatteringStrength);
    CHECK(look.clouds.coverage == effects::CloudSettings{}.coverage);
    CHECK(look.ocean.deepColor == effects::OceanSettings{}.deepColor);
    CHECK(look.sunColor == SceneSettings{}.sunColor);
}

TEST_CASE("body_look: fields in the block override, the rest keep defaults", "[body_look]")
{
    const auto look = types::ParseBodyLook(nlohmann::json::parse(R"({
        "render": {
            "atmosphere": {"wavelengths": [600, 520, 480], "intensity": 2.0},
            "clouds": {"coverage": 0.9, "enabled": false},
            "ocean": {"deepColor": [0.2, 0.0, 0.1], "emissive": 3.0, "frozen": 1.0},
            "star": {"color": [1.0, 0.5, 0.3]},
            "lighting": {"ambient": 0.1}
        }})"));
    CHECK(look.atmosphere.wavelengths == glm::vec3(600, 520, 480));
    CHECK_THAT(look.atmosphere.intensity, WithinAbs(2.0, 1e-6));
    CHECK(look.atmosphere.scatteringStrength == effects::AtmosphereSettings{}.scatteringStrength);
    CHECK_THAT(look.clouds.coverage, WithinAbs(0.9, 1e-6));
    CHECK_FALSE(look.clouds.enabled);
    CHECK(look.ocean.deepColor == glm::vec3(0.2f, 0.0f, 0.1f));
    CHECK_THAT(look.ocean.emissive, WithinAbs(3.0, 1e-6));
    CHECK_THAT(look.ocean.frozen, WithinAbs(1.0, 1e-6));
    CHECK(look.ocean.reflectivity == effects::OceanSettings{}.reflectivity);
    CHECK(look.sunColor == glm::vec3(1.0f, 0.5f, 0.3f));
    CHECK_THAT(look.lighting.ambientLight, WithinAbs(0.1, 1e-6));
}

TEST_CASE("body_look: palette and material tints", "[body_look]")
{
    const auto look = types::ParseBodyLook(nlohmann::json::parse(R"({
        "render": {"palette": [[1, 0, 1], [0, 1, 1], [9]], "materials": {"rockTint": [0.2, 0.2, 0.2]}}})"));
    REQUIRE(look.palette.size() == 2); // the malformed entry is skipped
    CHECK(look.palette[0] == glm::vec3(1.0f, 0.0f, 1.0f));
    CHECK(look.surface.rockTint == glm::vec3(0.2f));
    CHECK(look.surface.sandTint == glm::vec3(1.0f));
}

TEST_CASE("body_look: wrong types are ignored", "[body_look]")
{
    const auto look = types::ParseBodyLook(nlohmann::json::parse(R"({
        "render": {"clouds": {"coverage": "lots"}, "star": {"color": [1, 2]}}})"));
    CHECK(look.clouds.coverage == effects::CloudSettings{}.coverage);
    CHECK(look.sunColor == SceneSettings{}.sunColor);
}

TEST_CASE("body_look: apply keeps the light direction and exposure", "[body_look]")
{
    types::BodyLook look;
    look.sunColor = glm::vec3(1.0f, 0.2f, 0.2f);
    effects::AtmosphereSettings atmosphere;
    effects::CloudSettings clouds;
    effects::OceanSettings ocean;
    SceneSettings scene;
    SurfaceSettings surface;
    scene.lightDir = glm::vec3(0.0f, -1.0f, 0.0f);
    scene.exposure = 2.0f;
    types::ApplyBodyLook(look, atmosphere, clouds, ocean, scene, surface);
    CHECK(scene.sunColor == glm::vec3(1.0f, 0.2f, 0.2f));
    CHECK(scene.lightDir == glm::vec3(0.0f, -1.0f, 0.0f));
    CHECK(scene.exposure == 2.0f);
}
