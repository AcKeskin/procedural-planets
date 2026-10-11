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

// pg_generate_heights / pg_generate_shading see the same continent mask and macro field as
// the patch path, so the standalone calls give the patch's numbers for the same body, seed
// and directions.

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

// Earth with the macro world on, so both the mask and the macro field matter.
planetgen::BodyConfig EarthWithMacro()
{
    planetgen::BodyConfig cfg;
    const std::string path = std::string(PLANETGEN_TEST_DATA_DIR) + "/data/bodies/earth.json";
    REQUIRE(planetgen::BodyConfigLoadFile(path, cfg).empty());
    cfg.macroWorld.enabled = true;
    return cfg;
}

bool Same(const float* a, const float* b, size_t count)
{
    return std::memcmp(a, b, count * sizeof(float)) == 0;
}

} // namespace

TEST_CASE("standalone calls: heights and shading match the patch path", "[gpu][standalone]")
{
    const uint32_t seed = 42u;
    const uint32_t n = 2048;
    PgContext ctx = MakeCtx();
    REQUIRE(ctx != nullptr);
    {
        const auto cfg = EarthWithMacro();
        const std::string json = planetgen::BodyConfigToJson(cfg);
        PgBody body = pg_body_create_from_json_string(ctx, json.c_str());
        REQUIRE(body != nullptr);
        const auto verts = SphereVertices(n);

        auto& gpu = *ctx->backend;
        const auto hb = gpu.CreateBuffer(n * sizeof(float));
        const auto nb = gpu.CreateBuffer(n * 3 * sizeof(float));
        const auto sb = gpu.CreateBuffer(n * 4 * sizeof(float));
        REQUIRE(pg_generate_patch(body, verts.data(), n, seed, hb, nb, sb, nullptr) == PG_SUCCESS);
        std::vector<float> patchHeights(n), patchNormals(n * 3), patchShading(n * 4);
        gpu.DownloadBuffer(hb, patchHeights.data(), n * sizeof(float));
        gpu.DownloadBuffer(nb, patchNormals.data(), n * 3 * sizeof(float));
        gpu.DownloadBuffer(sb, patchShading.data(), n * 4 * sizeof(float));
        gpu.DestroyBuffer(hb);
        gpu.DestroyBuffer(nb);
        gpu.DestroyBuffer(sb);

        PgResult h = pg_generate_heights(body, verts.data(), n, seed);
        REQUIRE(h != nullptr);
        CHECK(Same(pg_result_get_heights(h), patchHeights.data(), n));
        CHECK(Same(pg_result_get_normals(h), patchNormals.data(), n * 3));
        pg_result_destroy(h);

        const PgShadingDesc shadingDesc = planetgen::ProjectToShadingDesc(cfg);
        PgResult s = pg_generate_shading(body, verts.data(), patchHeights.data(), n, seed, &shadingDesc);
        REQUIRE(s != nullptr);
        CHECK(Same(pg_result_get_shading(s), patchShading.data(), n * 4));
        pg_result_destroy(s);

        pg_body_destroy(body);
    }
    pg_context_destroy(ctx);
}
