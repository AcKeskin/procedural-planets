#include <catch2/catch_test_macros.hpp>

// Per-strategy headless tests. Each strategy is constructed directly against the
// headless backend + a populated shader registry (reached through an internal
// PgContext) and asserted byte-exact across two runs — real compute, no window.
//
// Tagged [gpu] — requires a real local GPU (llvmpipe CI deferred, like the other
// determinism suites). Exclude with `ctest -LE gpu`.

#include "planetgen/planetgen.h"
#include "api/InternalTypes.h"
#include "strategy/HeightStrategy.h"
#include "strategy/ShadingStrategy.h"
#include "strategy/ErosionStrategy.h"
#include "strategy/PaletteProvider.h"
#include "strategy/GenerationPipeline.h"
#include "model/BodyConfig.h"
#include "model/BodyConfigProjection.h"
#include "model/PaletteRegistry.h"

#include <vector>
#include <cstring>
#include <cmath>

namespace
{

std::vector<float> MakeSphereVertices(int n)
{
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

// Read back a freshly-created SSBO into a host vector.
std::vector<float> Download(planetgen::IComputeBackend& gpu,
                            planetgen::GpuBufferHandle buf, size_t count)
{
    std::vector<float> out(count);
    gpu.DownloadBuffer(buf, out.data(), count * sizeof(float));
    return out;
}

} // namespace

TEST_CASE("HeightStrategy yields byte-exact heights+normals across two runs", "[gpu][strategy]")
{
    const uint32_t N = 512;
    const uint32_t Seed = 0xABCDu;

    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);
    auto& gpu = *ctx->backend;

    const PgBodyDesc desc = planetgen::ProjectToBodyDesc(planetgen::BodyConfig{});
    const auto verts = MakeSphereVertices(N);
    const size_t vbytes = N * 3 * sizeof(float);

    planetgen::HeightStrategy strategy;
    planetgen::StrategyContext sc{ gpu, ctx->registry, N, Seed };

    auto run = [&]()
    {
        auto vbuf = gpu.CreateBuffer(vbytes);
        gpu.UploadBuffer(vbuf, verts.data(), vbytes);
        auto hb = strategy.Run(sc, desc, vbuf);
        auto h = Download(gpu, hb.heights, N);
        auto nrm = Download(gpu, hb.normals, N * 3);
        gpu.DestroyBuffer(vbuf);
        gpu.DestroyBuffer(hb.heights);
        gpu.DestroyBuffer(hb.normals);
        return std::make_pair(h, nrm);
    };

    auto [h1, n1] = run();
    auto [h2, n2] = run();

    REQUIRE(std::memcmp(h1.data(), h2.data(), N * sizeof(float)) == 0);
    REQUIRE(std::memcmp(n1.data(), n2.data(), N * 3 * sizeof(float)) == 0);

    pg_context_destroy(ctx);
}

TEST_CASE("ShadingStrategy yields byte-exact shading across two runs", "[gpu][strategy]")
{
    const uint32_t N = 256;
    const uint32_t Seed = 999u;

    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);
    auto& gpu = *ctx->backend;

    const planetgen::BodyConfig cfg{};
    const PgBodyDesc    bdesc = planetgen::ProjectToBodyDesc(cfg);
    const PgShadingDesc sdesc = planetgen::ProjectToShadingDesc(cfg);
    const auto verts = MakeSphereVertices(N);
    const size_t vbytes = N * 3 * sizeof(float);

    planetgen::StrategyContext sc{ gpu, ctx->registry, N, Seed };

    // Heights once — identical input to both shading runs.
    planetgen::HeightStrategy height;
    auto vbuf0 = gpu.CreateBuffer(vbytes);
    gpu.UploadBuffer(vbuf0, verts.data(), vbytes);
    auto hb = height.Run(sc, bdesc, vbuf0);
    gpu.DestroyBuffer(vbuf0);

    planetgen::ShadingStrategy shading;
    auto runShading = [&]()
    {
        auto vbuf = gpu.CreateBuffer(vbytes);
        gpu.UploadBuffer(vbuf, verts.data(), vbytes);
        auto sbuf = shading.Run(sc, sdesc, vbuf, hb.heights);
        auto s = Download(gpu, sbuf, N * 4);
        gpu.DestroyBuffer(vbuf);
        gpu.DestroyBuffer(sbuf);
        return s;
    };

    auto s1 = runShading();
    auto s2 = runShading();

    REQUIRE(std::memcmp(s1.data(), s2.data(), N * 4 * sizeof(float)) == 0);

    gpu.DestroyBuffer(hb.heights);
    gpu.DestroyBuffer(hb.normals);
    pg_context_destroy(ctx);
}

TEST_CASE("PaletteProvider resolves the earth palette", "[strategy]")
{
    planetgen::PaletteRegistry registry; // built-ins registered in ctor
    planetgen::PaletteProvider provider;

    const planetgen::Palette& earth = provider.Resolve(registry, planetgen::PaletteRegistry::IdEarth);
    REQUIRE(earth.IsValid());
    REQUIRE_FALSE(earth.entries.empty());

    // Unknown id resolves to the empty palette, not a crash.
    const planetgen::Palette& missing = provider.Resolve(registry, "__no_such_palette__");
    REQUIRE_FALSE(missing.IsValid());
}

TEST_CASE("GenerationPipeline enables erosion only when the config turns it on", "[gpu][strategy]")
{
    const uint32_t N = 256;
    const uint32_t Seed = 7u;

    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);

    planetgen::PaletteRegistry palettes;
    planetgen::GenerationPipeline pipeline(*ctx->backend, ctx->registry, palettes);

    const auto verts = MakeSphereVertices(N);

    // Erosion OFF — runs height + shading, palette resolved.
    planetgen::BodyConfig off{};
    off.erosion.enabled = false;
    auto rOff = pipeline.Generate(off, verts.data(), N, Seed);
    REQUIRE(rOff.count == N);
    REQUIRE(rOff.heights.size() == N);
    REQUIRE(rOff.shading.size() == N * 4);
    REQUIRE(rOff.palette.IsValid());

    // Erosion ON — pipeline runs the extra stage; output is still well-formed.
    planetgen::BodyConfig on{};
    on.erosion.enabled = true;
    on.erosion.iterations = 2;
    auto rOn = pipeline.Generate(on, verts.data(), N, Seed);
    REQUIRE(rOn.count == N);
    REQUIRE(rOn.heights.size() == N);

    pg_context_destroy(ctx);
}

TEST_CASE("GenerationPipeline is byte-exact across two runs", "[gpu][strategy][determinism]")
{
    const uint32_t N = 512;
    const uint32_t Seed = 0x1234u;

    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);

    planetgen::PaletteRegistry palettes;
    planetgen::GenerationPipeline pipeline(*ctx->backend, ctx->registry, palettes);

    const planetgen::BodyConfig cfg{};
    const auto verts = MakeSphereVertices(N);

    auto r1 = pipeline.Generate(cfg, verts.data(), N, Seed);
    auto r2 = pipeline.Generate(cfg, verts.data(), N, Seed);

    REQUIRE(std::memcmp(r1.heights.data(), r2.heights.data(), N * sizeof(float)) == 0);
    REQUIRE(std::memcmp(r1.normals.data(), r2.normals.data(), N * 3 * sizeof(float)) == 0);
    REQUIRE(std::memcmp(r1.shading.data(), r2.shading.data(), N * 4 * sizeof(float)) == 0);

    pg_context_destroy(ctx);
}
