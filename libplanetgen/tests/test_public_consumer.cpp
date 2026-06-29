#include <catch2/catch_test_macros.hpp>

// Single-surface consumer test (done-criterion 1). This TU is compiled with ONLY
// the public include dir on its include path — no libplanetgen/src. It links the
// built shared library and reaches generation purely through planetgen.h /
// planetgen.hpp. If anything here needed an internal header, the build would fail,
// which is exactly the contract: a consumer links the library and generates from a
// config + seed through documented public calls alone.

#include "planetgen/planetgen.hpp"

#include <cmath>
#include <cstring>
#include <vector>

namespace
{

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

TEST_CASE("A consumer generates per-vertex data through public headers only", "[gpu][consumer]")
{
    pg::Context ctx(/*useExistingContext*/ false);

    PgBodyDesc bd{};
    pg_body_desc_defaults(&bd);
    pg::Body body = ctx.CreateBody(bd);
    REQUIRE(body.IsValid());

    const uint32_t N = 128;
    auto verts = SphereVerts(N);

    pg::Result r = body.GenerateHeights(verts.data(), N, 42u);
    REQUIRE(r.IsValid());
    REQUIRE(r.Count() == N);
    REQUIRE(r.Heights() != nullptr);
    REQUIRE(r.Normals() != nullptr);
}

TEST_CASE("The C++ wrapper surfaces palette, mesh, and error message", "[gpu][consumer][wrapper]")
{
    pg::Context ctx(false);

    PgBodyDesc bd{};
    pg_body_desc_defaults(&bd);
    pg::Body body = ctx.CreateBody(bd);
    REQUIRE(body.IsValid());

    // Mesh convenience: vertices + indices + per-vertex data + palette in one call.
    pg::Result mesh = body.GenerateMesh(/*rings*/ 6, /*segments*/ 10, 1u);
    REQUIRE(mesh.IsValid());
    REQUIRE(mesh.Count() > 0);
    REQUIRE(mesh.IndexCount() > 0);
    REQUIRE(mesh.Vertices() != nullptr);
    REQUIRE(mesh.Indices() != nullptr);
    REQUIRE(mesh.PaletteCount() > 0);
    REQUIRE(mesh.Palette() != nullptr);

    // Error message reachable through the wrapper.
    pg::Body bad = ctx.CreateBodyFromJson("no/such/file.json");
    REQUIRE_FALSE(bad.IsValid());
    REQUIRE(ctx.GetError() != PG_SUCCESS);
    REQUIRE_FALSE(ctx.GetErrorMessage().empty());
}

TEST_CASE("Identical inputs yield byte-identical output through the public surface", "[gpu][consumer][determinism]")
{
    pg::Context ctx(false);

    PgBodyDesc bd{};
    pg_body_desc_defaults(&bd);
    pg::Body body = ctx.CreateBody(bd);
    REQUIRE(body.IsValid());

    const uint32_t N = 256;
    auto verts = SphereVerts(N);
    const uint32_t seed = 2024u;

    pg::Result a = body.GenerateHeights(verts.data(), N, seed);
    pg::Result b = body.GenerateHeights(verts.data(), N, seed);
    REQUIRE(a.IsValid());
    REQUIRE(b.IsValid());

    REQUIRE(std::memcmp(a.Heights(), b.Heights(), N * sizeof(float)) == 0);
    REQUIRE(std::memcmp(a.Normals(), b.Normals(), N * 3 * sizeof(float)) == 0);
}
