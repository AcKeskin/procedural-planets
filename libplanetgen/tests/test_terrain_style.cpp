#include <catch2/catch_test_macros.hpp>

#include "planetgen/planetgen.h"
#include "api/InternalTypes.h"
#include "strategy/GenerationPipeline.h"
#include "model/BodyConfig.h"
#include "model/BodyConfigJson.h"

#include <cmath>
#include <cstring>
#include <string>
#include <vector>

// Terrain styles (dunes, craters, terraces, canyons, spires): each one changes the terrain,
// is deterministic, and a patch still matches the whole mesh on the same vertices. "Off gives
// Earth byte-identical" is the macro_world baseline-hash test.

namespace
{

PgContext MakeCtx()
{
    PgContextDesc d{};
    d.use_existing_context = 0;
    return pg_context_create(&d);
}

std::vector<float> SphereVertices(int n)
{
    std::vector<float> v;
    v.reserve(n * 3);
    const float goldenAngle = 2.39996323f;
    for (int i = 0; i < n; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(n);
        const float inc = std::acos(1.0f - 2.0f * t);
        const float az = goldenAngle * static_cast<float>(i);
        v.push_back(std::sin(inc) * std::cos(az));
        v.push_back(std::cos(inc));
        v.push_back(std::sin(inc) * std::sin(az));
    }
    return v;
}

planetgen::BodyConfig LoadEarth()
{
    planetgen::BodyConfig cfg;
    const std::string path = std::string(PLANETGEN_TEST_DATA_DIR) + "/data/bodies/earth.json";
    REQUIRE(planetgen::BodyConfigLoadFile(path, cfg).empty());
    cfg.macroWorld.enabled = false;
    return cfg;
}

std::vector<float> Heights(PgContext ctx, const planetgen::BodyConfig& cfg, const std::vector<float>& verts)
{
    const uint32_t n = static_cast<uint32_t>(verts.size() / 3);
    planetgen::GpuTextureHandle maskTex = 0;
    uint64_t maskKey = 0;
    planetgen::GenerationPipeline pipeline(*ctx->backend, ctx->registry, ctx->palettes);
    auto r = pipeline.Generate(cfg, verts.data(), n, 42u, { &maskTex, &maskKey });
    REQUIRE(r.count == n);
    auto& gpu = *ctx->backend;
    gpu.DestroyBuffer(r.heightsGpuBuffer);
    gpu.DestroyBuffer(r.normalsGpuBuffer);
    gpu.DestroyBuffer(r.shadingGpuBuffer);
    if (maskTex)
        gpu.DestroyTexture(maskTex);
    return r.heights;
}

int CountDifferent(const std::vector<float>& a, const std::vector<float>& b)
{
    int count = 0;
    for (size_t i = 0; i < a.size(); ++i)
        count += a[i] != b[i];
    return count;
}

} // namespace

TEST_CASE("terrain_style: every style changes the terrain and is deterministic", "[gpu][terrain_style]")
{
    PgContext ctx = MakeCtx();
    REQUIRE(ctx != nullptr);
    {
        const auto verts = SphereVertices(4096);
        const auto earth = LoadEarth();
        const auto plain = Heights(ctx, earth, verts);

        struct Case
        {
            const char* name;
            float planetgen::TerrainStyleBlock::* field;
        };
        const Case cases[] = {
            {"dunes", &planetgen::TerrainStyleBlock::dunes},
            {"craters", &planetgen::TerrainStyleBlock::craters},
            {"terraces", &planetgen::TerrainStyleBlock::terraces},
            {"canyons", &planetgen::TerrainStyleBlock::canyons},
            {"spires", &planetgen::TerrainStyleBlock::spires},
        };
        for (const auto& c : cases)
        {
            INFO("style " << c.name);
            auto cfg = earth;
            cfg.terrainStyle.*c.field = 1.0f;
            const auto a = Heights(ctx, cfg, verts);
            const auto b = Heights(ctx, cfg, verts);
            CHECK(CountDifferent(a, b) == 0);
            CHECK(CountDifferent(a, plain) > 40);
        }
    }
    pg_context_destroy(ctx);
}

TEST_CASE("terrain_style: craters work on an airless body without land mask or ocean", "[gpu][terrain_style]")
{
    PgContext ctx = MakeCtx();
    REQUIRE(ctx != nullptr);
    {
        const auto verts = SphereVertices(4096);
        auto cfg = LoadEarth();
        cfg.tectonics.enabled = false;
        const auto plain = Heights(ctx, cfg, verts);
        cfg.terrainStyle.craters = 1.0f;
        const auto cratered = Heights(ctx, cfg, verts);
        // Bowls go down: some vertices end lower than without craters.
        int lower = 0;
        for (size_t i = 0; i < plain.size(); ++i)
            lower += cratered[i] < plain[i] - 1e-6f;
        CHECK(lower > 40);
    }
    pg_context_destroy(ctx);
}

TEST_CASE("terrain_style: land styles still apply without tectonics", "[gpu][terrain_style]")
{
    PgContext ctx = MakeCtx();
    REQUIRE(ctx != nullptr);
    {
        const auto verts = SphereVertices(4096);
        auto cfg = LoadEarth();
        cfg.tectonics.enabled = false;
        const auto plain = Heights(ctx, cfg, verts);
        cfg.terrainStyle.dunes = 1.0f;
        CHECK(CountDifferent(Heights(ctx, cfg, verts), plain) > 40);
    }
    pg_context_destroy(ctx);
}

TEST_CASE("terrain_style: json round trip keeps the style block", "[terrain_style]")
{
    planetgen::BodyConfig cfg;
    cfg.terrainStyle.dunes = 0.7f;
    cfg.terrainStyle.terraceSteps = 9.0f;
    const std::string json = planetgen::BodyConfigToJson(cfg);
    planetgen::BodyConfig back;
    REQUIRE(planetgen::BodyConfigFromJson(json, back).empty());
    CHECK(back.terrainStyle.dunes == 0.7f);
    CHECK(back.terrainStyle.terraceSteps == 9.0f);
    CHECK(back.terrainStyle.craters == 0.0f);
}
