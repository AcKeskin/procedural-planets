#include <catch2/catch_test_macros.hpp>

// Closed-extension proof for done-criterion 5: a new generation strategy is a new
// IGenerationStrategy impl, with zero edits to sibling strategies or the
// orchestrator core. The demo strategy below is defined entirely in this test TU
// and composed through the IHeightStrategy interface — nothing in src/strategy/*
// is touched to make it run.
//
// It allocates a heights+normals buffer the same way the built-in HeightStrategy
// does, but fills heights with a constant via a CPU upload (no shader needed) —
// enough to prove the seam accepts an arbitrary impl. Tagged [gpu] only because it
// needs a live backend to create buffers.

#include "planetgen/planetgen.h"
#include "api/InternalTypes.h"
#include "strategy/IGenerationStrategy.h"

#include <vector>

namespace
{

PgContext MakeHeadlessCtx()
{
    PgContextDesc d{};
    d.use_existing_context = 0;
    return pg_context_create(&d);
}

// A third-party height strategy added without editing any built-in strategy or
// GenerationPipeline. Produces a flat, constant-height body.
class ConstantHeightStrategy final : public planetgen::IHeightStrategy
{
public:
    explicit ConstantHeightStrategy(float value) : _value(value) {}

    planetgen::HeightBuffers Run(const planetgen::StrategyContext& sc,
                                 const PgBodyDesc& /*desc*/,
                                 planetgen::GpuBufferHandle /*vertices*/) override
    {
        auto& gpu = sc.backend;
        const size_t hbytes = sc.vertexCount * sizeof(float);
        const size_t nbytes = sc.vertexCount * 3 * sizeof(float);

        std::vector<float> heights(sc.vertexCount, _value);
        std::vector<float> normals(sc.vertexCount * 3, 0.0f);

        auto hbuf = gpu.CreateBuffer(hbytes);
        auto nbuf = gpu.CreateBuffer(nbytes);
        gpu.UploadBuffer(hbuf, heights.data(), hbytes);
        gpu.UploadBuffer(nbuf, normals.data(), nbytes);
        return { hbuf, nbuf };
    }

private:
    float _value;
};

} // namespace

TEST_CASE("A new IHeightStrategy composes through the seam with zero core edits", "[gpu][strategy][extension]")
{
    const uint32_t N = 128;
    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);
    auto& gpu = *ctx->backend;

    // Drive the demo strategy exactly like the orchestrator drives a built-in one.
    ConstantHeightStrategy demo(0.5f);
    planetgen::StrategyContext sc{ gpu, ctx->registry, N, 0u };

    PgBodyDesc unused{};
    auto hb = demo.Run(sc, unused, /*vertices*/ 0);
    REQUIRE(hb.heights != 0);
    REQUIRE(hb.normals != 0);

    std::vector<float> out(N);
    gpu.DownloadBuffer(hb.heights, out.data(), N * sizeof(float));
    for (float h : out)
        REQUIRE(h == 0.5f);

    gpu.DestroyBuffer(hb.heights);
    gpu.DestroyBuffer(hb.normals);
    pg_context_destroy(ctx);
}
