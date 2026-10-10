#include <catch2/catch_test_macros.hpp>

#include "planetgen/planetgen.h"
#include "api/InternalTypes.h"
#include "strategy/GenerationPipeline.h"
#include "model/BodyConfig.h"
#include "model/BodyConfigJson.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

// macro-world-layer proofs. All cases carry "macro_world" in the name so
// `ctest -R macro_world` selects them. GPU work goes through the library's own
// backend (ctx->backend); the test exe makes no raw GL calls.

namespace
{

PgContext MakeHeadlessCtx()
{
    PgContextDesc d{};
    d.use_existing_context = 0;
    return pg_context_create(&d);
}

std::vector<float> MakeSphereVertices(int n)
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

uint64_t Fnv1a(uint64_t h, const void* data, size_t bytes)
{
    const auto* p = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < bytes; ++i)
    {
        h ^= p[i];
        h *= 0x100000001b3ull;
    }
    return h;
}

planetgen::BodyConfig LoadEarth()
{
    planetgen::BodyConfig cfg;
    const std::string path = std::string(PLANETGEN_TEST_DATA_DIR) + "/data/bodies/earth.json";
    REQUIRE(planetgen::BodyConfigLoadFile(path, cfg).empty());
    return cfg;
}

// Hash of the whole-mesh Generate() output (heights + normals + shading).
uint64_t HashGenerate(PgContext ctx, const planetgen::BodyConfig& cfg, uint32_t seed)
{
    const int N = 4096;
    const auto verts = MakeSphereVertices(N);

    planetgen::GpuTextureHandle maskTex = 0;
    uint64_t maskKey = 0;
    planetgen::GenerationPipeline pipeline(*ctx->backend, ctx->registry, ctx->palettes);
    auto r = pipeline.Generate(cfg, verts.data(), N, seed, { &maskTex, &maskKey });
    REQUIRE(r.count == static_cast<uint32_t>(N));

    uint64_t h = 0xcbf29ce484222325ull;
    h = Fnv1a(h, r.heights.data(), r.heights.size() * sizeof(float));
    h = Fnv1a(h, r.normals.data(), r.normals.size() * sizeof(float));
    h = Fnv1a(h, r.shading.data(), r.shading.size() * sizeof(float));

    auto& gpu = *ctx->backend;
    gpu.DestroyBuffer(r.heightsGpuBuffer);
    gpu.DestroyBuffer(r.normalsGpuBuffer);
    gpu.DestroyBuffer(r.shadingGpuBuffer);
    if (maskTex)
        gpu.DestroyTexture(maskTex);
    return h;
}

} // namespace

TEST_CASE("macro_world off: Earth output matches the pre-macro baseline", "[gpu][macro_world]")
{
    // Dev-GPU baseline, recorded before any macro consumer existed. With the layer
    // off the height + shading output must stay byte-identical to it. A different
    // GPU/driver can legitimately produce other bits; re-record there.
    constexpr uint64_t kBaseline = 0xa21bec17651a82baull;

    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);

    const auto cfg = LoadEarth();
    REQUIRE_FALSE(cfg.macroWorld.enabled);

    const uint64_t h = HashGenerate(ctx, cfg, 42u);
    std::printf("macro_world off-parity hash: 0x%016llx\n", static_cast<unsigned long long>(h));
    CHECK(h == kBaseline);

    pg_context_destroy(ctx);
}
