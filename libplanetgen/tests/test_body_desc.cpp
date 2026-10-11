#include <catch2/catch_test_macros.hpp>

#include "planetgen/planetgen.h"
#include "api/InternalTypes.h"
#include "model/BodyConfig.h"
#include "model/BodyConfigJson.h"
#include "model/BodyConfigProjection.h"

#include <cmath>
#include <cstring>
#include <string>
#include <vector>

// pg_body_create(desc): the desc drives the body's config, so the mesh and patch paths
// (which read the config) see it, and a desc body matches the same body made from JSON.

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

planetgen::BodyConfig NonDefaultConfig()
{
    planetgen::BodyConfig cfg;
    cfg.shape.heightScale = 0.07f;
    cfg.shape.continentNoise.scale *= 1.7f;
    cfg.tectonics.numPlates = 5;
    return cfg;
}

std::vector<float> MeshHeights(PgBody body, uint32_t seed)
{
    PgResult r = pg_generate_mesh(body, 8, 16, seed);
    REQUIRE(r != nullptr);
    const float* h = pg_result_get_heights(r);
    std::vector<float> out(h, h + pg_result_get_count(r));
    pg_result_destroy(r);
    return out;
}

std::vector<float> PatchHeights(PgContext ctx, PgBody body, const std::vector<float>& verts, uint32_t seed)
{
    auto& gpu = *ctx->backend;
    const uint32_t n = static_cast<uint32_t>(verts.size() / 3);
    const auto hb = gpu.CreateBuffer(n * sizeof(float));
    const auto nb = gpu.CreateBuffer(n * 3 * sizeof(float));
    const auto sb = gpu.CreateBuffer(n * 4 * sizeof(float));
    REQUIRE(pg_generate_patch(body, verts.data(), n, seed, hb, nb, sb, nullptr) == PG_SUCCESS);
    std::vector<float> out(n);
    gpu.DownloadBuffer(hb, out.data(), n * sizeof(float));
    gpu.DestroyBuffer(hb);
    gpu.DestroyBuffer(nb);
    gpu.DestroyBuffer(sb);
    return out;
}

bool Same(const std::vector<float>& a, const std::vector<float>& b)
{
    return a.size() == b.size() && std::memcmp(a.data(), b.data(), a.size() * sizeof(float)) == 0;
}

} // namespace

TEST_CASE("body_desc: desc -> config -> desc gives the same desc", "[body_desc]")
{
    PgBodyDesc d{};
    pg_body_desc_defaults(&d);
    d.height_scale = 0.07f;
    d.use_tectonics = 0;
    d.num_plates = 5;
    d.trench_depth = 0.3f;
    d.style_craters = 0.5f;
    const PgBodyDesc back = planetgen::ProjectToBodyDesc(planetgen::BodyConfigFromDesc(d));
    CHECK(std::memcmp(&d, &back, sizeof(d)) == 0);
}

TEST_CASE("body_desc: a desc body generates like the same body from JSON", "[gpu][body_desc]")
{
    const uint32_t seed = 42u;
    PgContext ctx = MakeCtx();
    REQUIRE(ctx != nullptr);
    {
        const auto cfg = NonDefaultConfig();
        const PgBodyDesc desc = planetgen::ProjectToBodyDesc(cfg);
        PgBodyDesc defaults{};
        pg_body_desc_defaults(&defaults);
        const std::string json = planetgen::BodyConfigToJson(cfg);

        PgBody fromDesc = pg_body_create(ctx, &desc);
        PgBody fromJson = pg_body_create_from_json_string(ctx, json.c_str());
        PgBody plain = pg_body_create(ctx, &defaults);
        REQUIRE(fromDesc != nullptr);
        REQUIRE(fromJson != nullptr);
        REQUIRE(plain != nullptr);

        const auto meshDesc = MeshHeights(fromDesc, seed);
        CHECK(Same(meshDesc, MeshHeights(fromJson, seed)));
        CHECK_FALSE(Same(meshDesc, MeshHeights(plain, seed)));

        const auto verts = SphereVertices(512);
        const auto patchDesc = PatchHeights(ctx, fromDesc, verts, seed);
        CHECK(Same(patchDesc, PatchHeights(ctx, fromJson, verts, seed)));
        CHECK_FALSE(Same(patchDesc, PatchHeights(ctx, plain, verts, seed)));

        pg_body_destroy(fromDesc);
        pg_body_destroy(fromJson);
        pg_body_destroy(plain);
    }
    pg_context_destroy(ctx);
}
