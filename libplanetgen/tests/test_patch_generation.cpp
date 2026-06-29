#include <catch2/catch_test_macros.hpp>

#include "planetgen/planetgen.h"
#include "api/InternalTypes.h"
#include "strategy/GenerationPipeline.h"
#include "strategy/HeightStrategy.h"
#include "strategy/ShadingStrategy.h"
#include "strategy/ContinentMaskPass.h"
#include "model/BodyConfig.h"
#include "model/BodyConfigProjection.h"
#include "model/PaletteRegistry.h"
#include <vector>
#include <cstring>
#include <cmath>

// public-api v2 Steps 6+7: per-patch generation proofs.
//   Step 6 — per-patch determinism + GPU-resident-no-readback: GeneratePatch
//     writes into caller-owned SSBOs (no PgResult allocated, no fence); identical
//     inputs -> byte-identical buffer contents.
//   Step 7 — cross-consistency: a patch generated standalone is byte-identical to
//     the same vertices generated via the whole-mesh pipeline (the seam-free
//     proof), for the per-vertex stages (height + shading).
//   Plus a public-ABI status check on pg_generate_patch.
//
// Buffers + readback go through the library's OWN backend (ctx->backend) — its
// gl3w is the initialized one. The test makes NO raw GL calls (the test exe's
// gl3w table is never initialized), mirroring test_strategies.cpp.
//
// Tagged [gpu] so CI without a real GPU can exclude via `ctest -LE gpu`.

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

std::vector<float> Download(planetgen::IComputeBackend& gpu,
                            planetgen::GpuBufferHandle buf, size_t count)
{
    std::vector<float> out(count);
    gpu.DownloadBuffer(buf, out.data(), count * sizeof(float));
    return out;
}

} // namespace

TEST_CASE("GeneratePatch writes into caller-owned buffers, byte-identical across runs", "[gpu][patch]")
{
    const uint32_t N = 512;
    const uint32_t Seed = 0xC0FFEEu;

    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);
    auto& gpu = *ctx->backend;

    const planetgen::BodyConfig cfg{};
    const auto verts = MakeSphereVertices(N);

    planetgen::GenerationPipeline pipeline(gpu, ctx->registry, ctx->palettes);

    // Two independent caller-owned buffer sets; same inputs into each.
    auto run = [&]()
    {
        const auto h = gpu.CreateBuffer(N * sizeof(float));
        const auto n = gpu.CreateBuffer(N * 3 * sizeof(float));
        const auto s = gpu.CreateBuffer(N * 4 * sizeof(float));
        const bool ok = pipeline.GeneratePatch(cfg, verts.data(), N, Seed, h, n, s);
        REQUIRE(ok);
        auto outH = Download(gpu, h, N);
        auto outN = Download(gpu, n, N * 3);
        auto outS = Download(gpu, s, N * 4);
        gpu.DestroyBuffer(h);
        gpu.DestroyBuffer(n);
        gpu.DestroyBuffer(s);
        return std::make_tuple(outH, outN, outS);
    };

    auto [h1, n1, s1] = run();
    auto [h2, n2, s2] = run();

    REQUIRE(std::memcmp(h1.data(), h2.data(), N * sizeof(float)) == 0);
    REQUIRE(std::memcmp(n1.data(), n2.data(), N * 3 * sizeof(float)) == 0);
    REQUIRE(std::memcmp(s1.data(), s2.data(), N * 4 * sizeof(float)) == 0);

    pg_context_destroy(ctx);
}

TEST_CASE("A standalone patch is byte-identical to the same vertices in the whole mesh", "[gpu][patch]")
{
    // Whole-mesh result via the full pipeline, then the SAME vertices regenerated
    // as a patch — must match byte-for-byte (cross-consistency, the seam-free-LOD
    // proof). Scoped to per-vertex stages (height + shading); erosion is off the
    // per-patch path, and the default config has it disabled, so the whole-mesh
    // run here is also height->shading only — a like-for-like comparison.
    const uint32_t N = 384;
    const uint32_t Seed = 0x5EAB1ECEu;

    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);
    auto& gpu = *ctx->backend;

    planetgen::BodyConfig cfg{};
    REQUIRE_FALSE(cfg.erosion.enabled); // guard the like-for-like assumption

    const auto verts = MakeSphereVertices(N);

    planetgen::GenerationPipeline pipeline(gpu, ctx->registry, ctx->palettes);

    // Whole-mesh path (CPU readback into PipelineResult).
    const planetgen::PipelineResult whole = pipeline.Generate(cfg, verts.data(), N, Seed);
    REQUIRE(whole.count == N);

    // Per-patch path over the same vertices into caller-owned buffers.
    const auto ph = gpu.CreateBuffer(N * sizeof(float));
    const auto pn = gpu.CreateBuffer(N * 3 * sizeof(float));
    const auto ps = gpu.CreateBuffer(N * 4 * sizeof(float));
    const bool ok = pipeline.GeneratePatch(cfg, verts.data(), N, Seed, ph, pn, ps);
    REQUIRE(ok);

    const auto patchHeights = Download(gpu, ph, N);
    const auto patchNormals = Download(gpu, pn, N * 3);
    const auto patchShading = Download(gpu, ps, N * 4);

    REQUIRE(std::memcmp(whole.heights.data(), patchHeights.data(), N * sizeof(float)) == 0);
    REQUIRE(std::memcmp(whole.normals.data(), patchNormals.data(), N * 3 * sizeof(float)) == 0);
    REQUIRE(std::memcmp(whole.shading.data(), patchShading.data(), N * 4 * sizeof(float)) == 0);

    gpu.DestroyBuffer(ph);
    gpu.DestroyBuffer(pn);
    gpu.DestroyBuffer(ps);
    pg_context_destroy(ctx);
}

TEST_CASE("pg_generate_patch (public ABI) returns success and echoes the vertex count", "[gpu][patch]")
{
    // Exercises the C entry's status contract: caller-owned backend buffers in,
    // PG_SUCCESS + echoed count out, no result allocated. Readback is covered by
    // the pipeline-level tests above; here we only check the ABI surface.
    const uint32_t N = 256;
    const uint32_t Seed = 0x1234u;

    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);
    auto& gpu = *ctx->backend;

    PgBodyDesc bd{};
    pg_body_desc_defaults(&bd);
    PgBody body = pg_body_create(ctx, &bd);
    REQUIRE(body != nullptr);

    const auto verts = MakeSphereVertices(N);
    const auto h = gpu.CreateBuffer(N * sizeof(float));
    const auto n = gpu.CreateBuffer(N * 3 * sizeof(float));
    const auto s = gpu.CreateBuffer(N * 4 * sizeof(float));

    uint32_t echoed = 0;
    const PgError e = pg_generate_patch(body, verts.data(), N, Seed, h, n, s, &echoed);
    REQUIRE(e == PG_SUCCESS);
    REQUIRE(echoed == N);

    // Null buffer -> invalid argument, no crash.
    const PgError bad = pg_generate_patch(body, verts.data(), N, Seed, 0, n, s, nullptr);
    REQUIRE(bad == PG_ERROR_INVALID_ARGUMENT);

    gpu.DestroyBuffer(h);
    gpu.DestroyBuffer(n);
    gpu.DestroyBuffer(s);
    pg_body_destroy(body);
    pg_context_destroy(ctx);
}

TEST_CASE("Library-owned continent mask changes terrain, is deterministic, and is cached", "[gpu][patch][mask]")
{
    // Criterion 7: with a mask-enabled config the library bakes + samples the mask
    // (no caller texture); mask-on output differs from mask-off, is deterministic,
    // and the body caches the texture (a second call reuses it).
    const uint32_t N = 384;
    const uint32_t Seed = 0xA11CE5u;

    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);
    auto& gpu = *ctx->backend;

    planetgen::BodyConfig cfg{};
    REQUIRE(cfg.tectonics.enabled); // default enables the mask path

    const auto verts = MakeSphereVertices(N);
    planetgen::GenerationPipeline pipeline(gpu, ctx->registry, ctx->palettes);

    // Mask OFF: pass no cache -> ResolveMask returns 0 -> continentMaskAvailable=0.
    const planetgen::PipelineResult off = pipeline.Generate(cfg, verts.data(), N, Seed);
    REQUIRE(off.count == N);

    // Mask ON: a body-owned cache. First call bakes + caches.
    planetgen::GpuTextureHandle maskTex = 0;
    uint64_t maskKey = 0;
    const planetgen::GenerationPipeline::MaskCache cache{ &maskTex, &maskKey };

    const planetgen::PipelineResult on1 = pipeline.Generate(cfg, verts.data(), N, Seed, cache);
    REQUIRE(on1.count == N);
    REQUIRE(maskTex != 0);            // the library baked a mask, no caller texture
    const planetgen::GpuTextureHandle bakedOnce = maskTex;

    // Second call reuses the cached texture (no rebake -> same handle).
    const planetgen::PipelineResult on2 = pipeline.Generate(cfg, verts.data(), N, Seed, cache);
    REQUIRE(maskTex == bakedOnce);    // cache hit, not rebaked

    // Mask ON differs from mask OFF (the mask actually shapes terrain).
    REQUIRE(std::memcmp(off.heights.data(), on1.heights.data(), N * sizeof(float)) != 0);
    // Mask ON is deterministic across calls.
    REQUIRE(std::memcmp(on1.heights.data(), on2.heights.data(), N * sizeof(float)) == 0);

    gpu.DestroyTexture(maskTex);
    pg_context_destroy(ctx);
}

TEST_CASE("Cross-consistency holds with a mask-enabled config", "[gpu][patch][mask]")
{
    // Criterion 6 (evolved): a standalone patch == the whole-mesh region byte-for-byte
    // WITH the mask on. Both dispatches sample the SAME body-cached mask (a per-vertex
    // -pure texture lookup), so seam-free LOD survives the mask path.
    const uint32_t N = 384;
    const uint32_t Seed = 0xBEEF77u;

    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);
    auto& gpu = *ctx->backend;

    planetgen::BodyConfig cfg{};
    REQUIRE(cfg.tectonics.enabled);

    const auto verts = MakeSphereVertices(N);
    planetgen::GenerationPipeline pipeline(gpu, ctx->registry, ctx->palettes);

    // One shared mask cache for both paths (as the real body would have).
    planetgen::GpuTextureHandle maskTex = 0;
    uint64_t maskKey = 0;
    const planetgen::GenerationPipeline::MaskCache cache{ &maskTex, &maskKey };

    const planetgen::PipelineResult whole = pipeline.Generate(cfg, verts.data(), N, Seed, cache);
    REQUIRE(whole.count == N);
    REQUIRE(maskTex != 0);

    const auto ph = gpu.CreateBuffer(N * sizeof(float));
    const auto pn = gpu.CreateBuffer(N * 3 * sizeof(float));
    const auto ps = gpu.CreateBuffer(N * 4 * sizeof(float));
    const bool ok = pipeline.GeneratePatch(cfg, verts.data(), N, Seed, ph, pn, ps, cache);
    REQUIRE(ok);

    const auto patchHeights = Download(gpu, ph, N);
    const auto patchNormals = Download(gpu, pn, N * 3);
    const auto patchShading = Download(gpu, ps, N * 4);

    REQUIRE(std::memcmp(whole.heights.data(), patchHeights.data(), N * sizeof(float)) == 0);
    REQUIRE(std::memcmp(whole.normals.data(), patchNormals.data(), N * 3 * sizeof(float)) == 0);
    REQUIRE(std::memcmp(whole.shading.data(), patchShading.data(), N * 4 * sizeof(float)) == 0);

    gpu.DestroyBuffer(ph);
    gpu.DestroyBuffer(pn);
    gpu.DestroyBuffer(ps);
    gpu.DestroyTexture(maskTex);
    pg_context_destroy(ctx);
}
