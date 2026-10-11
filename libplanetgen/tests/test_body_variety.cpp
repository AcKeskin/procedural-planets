#include <catch2/catch_test_macros.hpp>

#include "planetgen/planetgen.h"
#include "api/InternalTypes.h"
#include "planetgen/model/BodyConfigJson.h"
#include "planetgen/model/BodyVariety.h"

#include <fstream>
#include <string>

using nlohmann::json;

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
    CHECK(planetgen::RollVariety(kBody, 7u) == planetgen::RollVariety(kBody, 7u));
}

TEST_CASE("body_variety: different seeds give different worlds", "[body_variety]")
{
    int different = 0;
    const json first = planetgen::RollVariety(kBody, 1u);
    for (uint32_t seed = 2; seed < 10; ++seed)
        different += planetgen::RollVariety(kBody, seed) != first;
    CHECK(different >= 7);
}

TEST_CASE("body_variety: rolled values stay inside their rules", "[body_variety]")
{
    for (uint32_t seed = 0; seed < 50; ++seed)
    {
        const json w = planetgen::RollVariety(kBody, seed);
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

TEST_CASE("body_variety: adding a render.surface key does not change the other rolls", "[body_variety]")
{
    json withSurface = kBody;
    withSurface["variety"]["render.surface.rock.slope"] = json::parse(R"({"range": [0.1, 0.2]})");
    for (uint32_t seed = 0; seed < 10; ++seed)
    {
        json a = planetgen::RollVariety(kBody, seed);
        json b = planetgen::RollVariety(withSurface, seed);
        b["render"].erase("surface");
        b.erase("variety");
        a.erase("variety");
        CHECK(a == b);
    }
}

TEST_CASE("body_variety: no variety block or a bad rule leaves the body alone", "[body_variety]")
{
    const json plain = json::parse(R"({"shape": {"heightScale": 0.03}})");
    CHECK_FALSE(planetgen::HasVariety(plain));
    CHECK(planetgen::RollVariety(plain, 3u) == plain);

    const json bad = json::parse(R"({"shape": {"heightScale": 0.03},
        "variety": {"shape.heightScale": [0.1, 0.2], "shape.x": {"range": ["a", 1]}}})");
    CHECK(planetgen::RollVariety(bad, 3u)["shape"] == plain["shape"]);
}

// The library rolls a planet type itself: any user with the same JSON and seed gets the
// same world as the app, which rolls the same JSON with the same function.
TEST_CASE("body_variety: a seeded JSON body is the rolled world", "[body_variety]")
{
    const std::string path = std::string(PLANETGEN_TEST_DATA_DIR) + "/data/bodies/dune.json";
    std::ifstream in(path);
    const json dune = json::parse(in);
    REQUIRE(planetgen::HasVariety(dune));

    PgContextDesc d{};
    PgContext ctx = pg_context_create(&d);
    REQUIRE(ctx != nullptr);

    auto configJson = [&](PgBody body)
    {
        REQUIRE(body != nullptr);
        const std::string out = planetgen::BodyConfigToJson(body->config);
        pg_body_destroy(body);
        return out;
    };
    auto rolled = [&](uint32_t seed)
    {
        planetgen::BodyConfig cfg;
        REQUIRE(planetgen::BodyConfigFromJson(planetgen::RollVariety(dune, seed).dump(), cfg).empty());
        return planetgen::BodyConfigToJson(cfg);
    };

    const std::string seeded = configJson(pg_body_create_from_json_seeded(ctx, path.c_str(), 11u));
    CHECK(seeded == rolled(11u));
    CHECK(seeded == configJson(pg_body_create_from_json_string_seeded(ctx, dune.dump().c_str(), 11u)));

    int different = 0;
    for (uint32_t seed = 12; seed < 20; ++seed)
        different += configJson(pg_body_create_from_json_seeded(ctx, path.c_str(), seed)) != seeded;
    CHECK(different > 0);

    // The plain variant keeps the base values.
    planetgen::BodyConfig base;
    REQUIRE(planetgen::BodyConfigFromJson(dune.dump(), base).empty());
    CHECK(configJson(pg_body_create_from_json(ctx, path.c_str())) == planetgen::BodyConfigToJson(base));

    CHECK(pg_body_create_from_json_seeded(ctx, "missing.json", 1u) == nullptr);
    CHECK(pg_context_get_error(ctx) == PG_ERROR_FILE_NOT_FOUND);
    CHECK(pg_body_create_from_json_string_seeded(ctx, "{not json", 1u) == nullptr);
    CHECK(pg_context_get_error(ctx) == PG_ERROR_JSON_PARSE_FAILED);

    pg_context_destroy(ctx);
}
