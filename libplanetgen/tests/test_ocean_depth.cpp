#include <catch2/catch_test_macros.hpp>

#include "planetgen/planetgen.h"
#include "api/InternalTypes.h"
#include "strategy/GenerationPipeline.h"
#include "planetgen/model/BodyConfig.h"
#include "planetgen/model/BodyConfigJson.h"

#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// The app's ocean pass (shaders/ocean.vert) takes its water depth from shaders/includes/
// ocean_depth.glsl, fed the same patch height buffer the terrain draws. This runs that include
// on a real GeneratePatch height buffer (continent mask + macro world on) and checks the depth
// is exactly seaLevel - height, so the ocean coast is the drawn coast.

namespace
{

PgContext MakeHeadlessCtx()
{
    PgContextDesc d{};
    d.use_existing_context = 0;
    return pg_context_create(&d);
}

std::string ReadFile(const std::string& relPath)
{
    std::ifstream f(std::string(PLANETGEN_TEST_DATA_DIR) + "/" + relPath);
    REQUIRE(f.is_open());
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Same binding as the ocean vertex shader's PatchHeight buffer; writes depth for morph 0 and 1.
// kSeaLevel is prepended by the test (the backend has no loose-uniform setter).
const char* kDepthProbe = R"(
layout(local_size_x = 64) in;
layout(std430, binding = 1) readonly buffer PatchHeight { float height[]; };
layout(std430, binding = 4) writeonly buffer Depths { vec2 depth[]; };
void main()
{
    uint id = gl_GlobalInvocationID.x;
    if (id >= uint(height.length())) return;
    float h = height[id];
    float coarse = h + 0.001; // stand-in morph target
    depth[id] = vec2(oceanDepth(kSeaLevel, h, coarse, 0.0), oceanDepth(kSeaLevel, h, coarse, 1.0));
}
)";

} // namespace

TEST_CASE("ocean_depth: ocean depth is sea level minus the drawn terrain height per vertex", "[gpu][ocean]")
{
    const uint32_t N = 1024;
    const float SeaLevel = 1.0f;

    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);
    auto& gpu = *ctx->backend;
    {
        planetgen::BodyConfig cfg;
        REQUIRE(planetgen::BodyConfigLoadFile(std::string(PLANETGEN_TEST_DATA_DIR) + "/data/bodies/earth.json", cfg).empty());
        cfg.macroWorld.enabled = true;
        cfg.macroWorld.resolution = 128;

        // A strip of directions long enough to cross a coastline.
        std::vector<float> verts;
        for (uint32_t i = 0; i < N; ++i)
        {
            const float lon = 3.0f * static_cast<float>(i) / N;
            const float x = std::cos(lon), y = 0.1f, z = std::sin(lon);
            const float l = std::sqrt(x * x + y * y + z * z);
            verts.insert(verts.end(), { x / l, y / l, z / l });
        }

        planetgen::GpuTextureHandle maskTex = 0;
        uint64_t maskKey = 0;
        planetgen::GenerationPipeline pipeline(gpu, ctx->registry, ctx->palettes);
        const auto hBuf = gpu.CreateBuffer(N * sizeof(float));
        const auto nBuf = gpu.CreateBuffer(N * 3 * sizeof(float));
        const auto sBuf = gpu.CreateBuffer(N * 4 * sizeof(float));
        REQUIRE(pipeline.GeneratePatch(cfg, verts.data(), N, 42u, hBuf, nBuf, sBuf,
                                       { &maskTex, &maskKey }, { &ctx->macroTexture, &ctx->macroBuiltKey }));
        REQUIRE(ctx->macroTexture != 0);

        const std::string src = "#version 450\nconst float kSeaLevel = " + std::to_string(SeaLevel) + ";\n" +
                                ReadFile("shaders/includes/ocean_depth.glsl") + kDepthProbe;
        REQUIRE(ctx->registry.RegisterProgram("test_ocean_depth_probe", src));
        const uint32_t program = ctx->registry.Resolve("test_ocean_depth_probe");

        const auto dBuf = gpu.CreateBuffer(N * 2 * sizeof(float));
        gpu.BindShader(program);
        gpu.BindBuffer(hBuf, 1);
        gpu.BindBuffer(dBuf, 4);
        gpu.Dispatch((N + 63) / 64);
        gpu.Barrier();

        std::vector<float> heights(N), depths(N * 2);
        gpu.DownloadBuffer(hBuf, heights.data(), N * sizeof(float));
        gpu.DownloadBuffer(dBuf, depths.data(), depths.size() * sizeof(float));

        int wet = 0, dry = 0, mismatches = 0;
        for (uint32_t i = 0; i < N; ++i)
        {
            const float expected = SeaLevel - heights[i];
            const float expectedMorphed = SeaLevel - (heights[i] + 0.001f);
            mismatches += std::fabs(depths[i * 2] - expected) > 1e-6f;
            mismatches += std::fabs(depths[i * 2 + 1] - expectedMorphed) > 1e-6f;
            (expected > 0.0f ? wet : dry) += 1;
        }
        INFO(wet << " underwater vertices, " << dry << " on land");
        CHECK(mismatches == 0);
        CHECK(wet > 0); // the strip crosses a coast: both sides present
        CHECK(dry > 0);

        for (auto b : { hBuf, nBuf, sBuf, dBuf })
            gpu.DestroyBuffer(b);
        gpu.DestroyTexture(maskTex);
    }
    pg_context_destroy(ctx);
}
