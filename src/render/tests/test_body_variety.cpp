#include <catch2/catch_test_macros.hpp>

#include "types/BodyVariety.h"

using nlohmann::json;
using namespace planets::render;

namespace
{
const json kBody = json::parse(R"({
    "shape": {"heightScale": 0.03},
    "render": {"star": {"class": "yellow"}},
    "variety": {
        "shape.heightScale": {"range": [0.02, 0.05]},
        "shape.continentCount": {"range": [3, 9]},
        "render.star.class": {"pick": ["red", "orange", "white"]},
        "render.atmosphere.wavelengths": {"mix": [[680, 550, 440], [600, 560, 520]]},
        "terrainStyle.dunes": {"range": [0.5, 1.0]}
    }})");
}

TEST_CASE("body_variety: the same seed rolls the same world", "[body_variety]")
{
    CHECK(types::RollVariety(kBody, 7u) == types::RollVariety(kBody, 7u));
}

TEST_CASE("body_variety: different seeds give different worlds", "[body_variety]")
{
    int different = 0;
    const json first = types::RollVariety(kBody, 1u);
    for (uint32_t seed = 2; seed < 10; ++seed)
        different += types::RollVariety(kBody, seed) != first;
    CHECK(different >= 7);
}

TEST_CASE("body_variety: rolled values stay inside their rules", "[body_variety]")
{
    for (uint32_t seed = 0; seed < 50; ++seed)
    {
        const json w = types::RollVariety(kBody, seed);
        const float h = w["shape"]["heightScale"].get<float>();
        CHECK(h >= 0.02f);
        CHECK(h <= 0.05f);
        REQUIRE(w["shape"]["continentCount"].is_number_integer());
        const int count = w["shape"]["continentCount"].get<int>();
        CHECK(count >= 3);
        CHECK(count <= 9);
        const std::string star = w["render"]["star"]["class"].get<std::string>();
        CHECK((star == "red" || star == "orange" || star == "white"));
        const float blue = w["render"]["atmosphere"]["wavelengths"][2].get<float>();
        CHECK(blue >= 440.0f);
        CHECK(blue <= 520.0f);
        CHECK(w["terrainStyle"]["dunes"].get<float>() >= 0.5f); // created the missing block
    }
}

TEST_CASE("body_variety: no variety block or a bad rule leaves the body alone", "[body_variety]")
{
    const json plain = json::parse(R"({"shape": {"heightScale": 0.03}})");
    CHECK_FALSE(types::HasVariety(plain));
    CHECK(types::RollVariety(plain, 3u) == plain);

    const json bad = json::parse(R"({"shape": {"heightScale": 0.03},
        "variety": {"shape.heightScale": [0.1, 0.2], "shape.x": {"range": ["a", 1]}}})");
    CHECK(types::RollVariety(bad, 3u)["shape"] == plain["shape"]);
}
