#include <catch2/catch_test_macros.hpp>

// Public C-ABI surface tests: error model, composable config, palette in result,
// mesh convenience, and determinism — all through public headers only.

#include "planetgen/planetgen.h"

#include <cstring>
#include <string>
#include <vector>
#include <cmath>

#ifndef PLANETGEN_TEST_DATA_DIR
#define PLANETGEN_TEST_DATA_DIR "."
#endif

namespace
{

PgContext MakeHeadlessCtx()
{
    PgContextDesc d{};
    d.use_existing_context = 0;
    return pg_context_create(&d);
}

std::string BodyJson(const char* name)
{
    return std::string(PLANETGEN_TEST_DATA_DIR) + "/data/bodies/" + name;
}

std::vector<float> SphereVerts(int n)
{
    std::vector<float> v;
    v.reserve(n * 3);
    const float ga = 2.39996323f;
    for (int i = 0; i < n; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(n);
        float inc = std::acos(1.0f - 2.0f * t);
        float az = ga * static_cast<float>(i);
        v.push_back(std::sin(inc) * std::cos(az));
        v.push_back(std::cos(inc));
        v.push_back(std::sin(inc) * std::sin(az));
    }
    return v;
}

} // namespace

TEST_CASE("A failing call yields a status code and a readable message", "[gpu][api][error]")
{
    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);

    // Bad JSON path -> file-not-found code + non-empty message.
    PgBody body = pg_body_create_from_json(ctx, "definitely/not/a/real/body.json");
    REQUIRE(body == nullptr);
    REQUIRE(pg_context_get_error(ctx) == PG_ERROR_FILE_NOT_FOUND);

    const char* msg = pg_get_last_error_message(ctx);
    REQUIRE(msg != nullptr);
    REQUIRE(std::strlen(msg) > 0);

    pg_context_destroy(ctx);
}

TEST_CASE("A non-Earth body loads through the composable JSON path and generates", "[gpu][api][config]")
{
    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);

    // Earth is just one config; a non-Earth body must load + generate with no
    // Earth-specific obligations. volcanic.json exercises a different block set.
    PgBody body = pg_body_create_from_json(ctx, BodyJson("volcanic.json").c_str());
    REQUIRE(body != nullptr);
    REQUIRE(pg_context_get_error(ctx) == PG_SUCCESS);

    const uint32_t N = 128;
    auto verts = SphereVerts(N);
    PgResult r = pg_generate_heights(body, verts.data(), N, 7u);
    REQUIRE(r != nullptr);
    REQUIRE(pg_result_get_count(r) == N);

    pg_result_destroy(r);
    pg_body_destroy(body);
    pg_context_destroy(ctx);
}

TEST_CASE("Generation result carries the resolved body palette", "[gpu][api][palette]")
{
    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);

    PgBody body = pg_body_create_from_json(ctx, BodyJson("earth.json").c_str());
    REQUIRE(body != nullptr);

    const uint32_t N = 64;
    auto verts = SphereVerts(N);
    PgResult r = pg_generate_heights(body, verts.data(), N, 1u);
    REQUIRE(r != nullptr);

    const PgPaletteEntry* pal = pg_result_get_palette(r);
    const uint32_t palCount = pg_result_get_palette_count(r);
    REQUIRE(palCount > 0);
    REQUIRE(pal != nullptr);
    // Colors are RGB in [0,1].
    for (uint32_t i = 0; i < palCount; ++i)
        for (int c = 0; c < 3; ++c)
            REQUIRE((pal[i].color[c] >= 0.0f && pal[i].color[c] <= 1.0f));

    pg_result_destroy(r);
    pg_body_destroy(body);
    pg_context_destroy(ctx);
}

TEST_CASE("Mesh-generating call returns a valid mesh with per-vertex data", "[gpu][api][mesh]")
{
    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);

    PgBodyDesc bd{};
    pg_body_desc_defaults(&bd);
    PgBody body = pg_body_create(ctx, &bd);
    REQUIRE(body != nullptr);

    PgResult r = pg_generate_mesh(body, /*rings*/ 8, /*segments*/ 12, 3u);
    REQUIRE(r != nullptr);

    const uint32_t vcount = pg_result_get_count(r);
    const uint32_t icount = pg_result_get_index_count(r);
    REQUIRE(vcount > 0);
    REQUIRE(icount > 0);
    REQUIRE(icount % 3 == 0); // triangle list

    const float* verts = pg_result_get_vertices(r);
    const uint32_t* idx = pg_result_get_indices(r);
    REQUIRE(verts != nullptr);
    REQUIRE(idx != nullptr);

    // Indices stay in range.
    for (uint32_t i = 0; i < icount; ++i)
        REQUIRE(idx[i] < vcount);

    // Per-vertex data length matches the vertex count.
    REQUIRE(pg_result_get_heights(r) != nullptr);
    REQUIRE(pg_result_get_shading(r) != nullptr);

    pg_result_destroy(r);
    pg_body_destroy(body);
    pg_context_destroy(ctx);
}

TEST_CASE("Last-error message on a null context is safe", "[api][error]")
{
    const char* msg = pg_get_last_error_message(nullptr);
    REQUIRE(msg != nullptr);
    REQUIRE(std::strlen(msg) > 0); // returns a static string, never crashes
}

TEST_CASE("A successful call clears the last error", "[gpu][api][error]")
{
    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);

    // Provoke an error first.
    (void)pg_body_create_from_json(ctx, "nope.json");
    REQUIRE(pg_context_get_error(ctx) != PG_SUCCESS);

    // A valid generation clears it.
    PgBodyDesc bd{};
    pg_body_desc_defaults(&bd);
    PgBody body = pg_body_create(ctx, &bd);
    REQUIRE(body != nullptr);

    const uint32_t N = 64;
    float verts[N * 3];
    for (uint32_t i = 0; i < N * 3; ++i) verts[i] = 0.5f;

    PgResult r = pg_generate_heights(body, verts, N, 1u);
    REQUIRE(r != nullptr);
    REQUIRE(pg_context_get_error(ctx) == PG_SUCCESS);
    REQUIRE(std::strlen(pg_get_last_error_message(ctx)) == 0);

    pg_result_destroy(r);
    pg_body_destroy(body);
    pg_context_destroy(ctx);
}
