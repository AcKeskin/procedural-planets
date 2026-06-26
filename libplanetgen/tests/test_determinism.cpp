#include <catch2/catch_test_macros.hpp>

#include "planetgen/planetgen.h"
#include <vector>
#include <cstring>

// Step 9: Determinism harness.
// Runs height (and shading) passes twice with identical inputs on a headless backend
// and asserts byte-identical output buffers.
//
// ## Notes
// Software-GL / llvmpipe CI is deferred — these tests require a real local GPU.
// Tag them [gpu][determinism] so CI can exclude with `ctest -LE gpu`.

namespace
{

std::vector<float> MakeSphereVertices(int n)
{
    // Evenly-ish distributed points on unit sphere via golden-angle spiral
    std::vector<float> v;
    v.reserve(n * 3);
    const float goldenAngle = 2.39996323f;
    for (int i = 0; i < n; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(n);
        float inc = std::acos(1.0f - 2.0f * t);
        float az  = goldenAngle * static_cast<float>(i);
        v.push_back(std::sin(inc) * std::cos(az));
        v.push_back(std::cos(inc));
        v.push_back(std::sin(inc) * std::sin(az));
    }
    return v;
}

PgContext MakeHeadlessCtx()
{
    PgContextDesc d{};
    d.use_existing_context = 0;
    return pg_context_create(&d);
}

} // namespace

TEST_CASE("Height pass is byte-identical across two runs", "[gpu][determinism]")
{
    const uint32_t VertexCount = 512;
    const uint32_t Seed = 0xDEADBEEFu;

    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);

    PgBodyDesc bd{};
    pg_body_desc_defaults(&bd);
    PgBody body = pg_body_create(ctx, &bd);
    REQUIRE(body != nullptr);

    auto verts = MakeSphereVertices(VertexCount);

    // First run
    PgResult r1 = pg_generate_heights(body, verts.data(), VertexCount, Seed);
    REQUIRE(r1 != nullptr);

    std::vector<float> h1(pg_result_get_heights(r1), pg_result_get_heights(r1) + VertexCount);
    std::vector<float> n1(pg_result_get_normals(r1), pg_result_get_normals(r1) + VertexCount * 3);
    pg_result_destroy(r1);

    // Second run — identical inputs
    PgResult r2 = pg_generate_heights(body, verts.data(), VertexCount, Seed);
    REQUIRE(r2 != nullptr);

    std::vector<float> h2(pg_result_get_heights(r2), pg_result_get_heights(r2) + VertexCount);
    std::vector<float> n2(pg_result_get_normals(r2), pg_result_get_normals(r2) + VertexCount * 3);
    pg_result_destroy(r2);

    // Byte-identical comparison
    REQUIRE(std::memcmp(h1.data(), h2.data(), VertexCount * sizeof(float)) == 0);
    REQUIRE(std::memcmp(n1.data(), n2.data(), VertexCount * 3 * sizeof(float)) == 0);

    pg_body_destroy(body);
    pg_context_destroy(ctx);
}

TEST_CASE("Shading pass is byte-identical across two runs", "[gpu][determinism]")
{
    const uint32_t VertexCount = 256;
    const uint32_t Seed = 12345u;

    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);

    PgBodyDesc bd{};
    pg_body_desc_defaults(&bd);
    PgBody body = pg_body_create(ctx, &bd);
    REQUIRE(body != nullptr);

    PgShadingDesc sd{};
    pg_shading_desc_defaults(&sd);

    auto verts = MakeSphereVertices(VertexCount);

    // Generate heights for shading input (not testing these for determinism here,
    // but they're identical inputs to both shading runs below)
    PgResult hr = pg_generate_heights(body, verts.data(), VertexCount, Seed);
    REQUIRE(hr != nullptr);
    // Copy heights to a local buffer so we can reuse for both shading calls
    std::vector<float> heights(pg_result_get_heights(hr),
                                pg_result_get_heights(hr) + VertexCount);
    pg_result_destroy(hr);

    // First shading run
    PgResult s1 = pg_generate_shading(body, verts.data(), heights.data(), VertexCount, Seed, &sd);
    REQUIRE(s1 != nullptr);
    std::vector<float> shad1(pg_result_get_shading(s1), pg_result_get_shading(s1) + VertexCount * 4);
    pg_result_destroy(s1);

    // Second shading run
    PgResult s2 = pg_generate_shading(body, verts.data(), heights.data(), VertexCount, Seed, &sd);
    REQUIRE(s2 != nullptr);
    std::vector<float> shad2(pg_result_get_shading(s2), pg_result_get_shading(s2) + VertexCount * 4);
    pg_result_destroy(s2);

    REQUIRE(std::memcmp(shad1.data(), shad2.data(), VertexCount * 4 * sizeof(float)) == 0);

    pg_body_destroy(body);
    pg_context_destroy(ctx);
}
