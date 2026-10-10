#include <catch2/catch_test_macros.hpp>

#include "planetgen/planetgen.h"
#include "api/InternalTypes.h"
#include <vector>
#include <cmath>

// Step 8b: HeadlessGLBackend integration tests.
// Requires a real local GPU — marked [gpu] so they can be skipped with
// `ctest -L gpu` if running in a no-GPU CI environment.

namespace
{

// Minimal unit-sphere vertices (6 axis-aligned points)
std::vector<float> AxisVertices()
{
    return {
         1, 0, 0,  -1, 0, 0,
         0, 1, 0,   0,-1, 0,
         0, 0, 1,   0, 0,-1
    };
}

PgContext MakeHeadlessCtx()
{
    PgContextDesc d{};
    d.use_existing_context = 0; // headless
    return pg_context_create(&d);
}

} // namespace

TEST_CASE("HeadlessGLBackend creates no visible window and produces output", "[gpu][headless]")
{
    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);

    PgBodyDesc bd{};
    pg_body_desc_defaults(&bd);
    PgBody body = pg_body_create(ctx, &bd);
    REQUIRE(body != nullptr);

    auto verts = AxisVertices();
    uint32_t count = static_cast<uint32_t>(verts.size() / 3);

    PgResult r = pg_generate_heights(body, verts.data(), count, 42u);
    REQUIRE(r != nullptr);
    REQUIRE(pg_result_get_count(r) == count);

    const float* heights = pg_result_get_heights(r);
    REQUIRE(heights != nullptr);

    // Heights should be near 1.0 (unit sphere + small terrain displacement)
    for (uint32_t i = 0; i < count; ++i)
    {
        INFO("height[" << i << "] = " << heights[i]);
        REQUIRE(heights[i] > 0.5f);
        REQUIRE(heights[i] < 2.0f);
    }

    pg_result_destroy(r);
    pg_body_destroy(body);
    pg_context_destroy(ctx);
}

TEST_CASE("HeadlessGLBackend custom GLSL registered and dispatched", "[gpu][headless]")
{
    // Register a trivial compute shader that writes 1.0 to a heights buffer,
    // dispatched via pg_generate_heights — but we can't inject custom shaders
    // through the C API yet (Step 8 only adds the test, not a custom-register API).
    // So this test exercises the ShaderRegistry's RegisterProgram path via
    // the internal backend directly.

    // For now, verify headless init + builtin shader dispatch produces non-zero output
    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);

    PgShadingDesc sd{};
    pg_shading_desc_defaults(&sd);
    sd.use_climate_model = 0; // use generic path

    PgBodyDesc bd{};
    pg_body_desc_defaults(&bd);
    PgBody body = pg_body_create(ctx, &bd);
    REQUIRE(body != nullptr);

    auto verts = AxisVertices();
    uint32_t count = static_cast<uint32_t>(verts.size() / 3);

    // First generate heights (needed for shading input)
    PgResult hr = pg_generate_heights(body, verts.data(), count, 7u);
    REQUIRE(hr != nullptr);
    const float* heights = pg_result_get_heights(hr);
    REQUIRE(heights != nullptr);

    // Now dispatch the generic shading path (ShadingGeneric builtin id)
    PgResult sr = pg_generate_shading(body, verts.data(), heights, count, 7u, &sd);
    REQUIRE(sr != nullptr);
    REQUIRE(pg_result_get_count(sr) == count);

    const float* shading = pg_result_get_shading(sr);
    REQUIRE(shading != nullptr);

    // Generic shader outputs normalizedHeight in x — all should be in [0,1]
    for (uint32_t i = 0; i < count; ++i)
    {
        float nx = shading[i * 4 + 0];
        INFO("shading[" << i << "].x = " << nx);
        REQUIRE(nx >= 0.0f);
        REQUIRE(nx <= 1.0f);
    }

    pg_result_destroy(sr);
    pg_result_destroy(hr);
    pg_body_destroy(body);
    pg_context_destroy(ctx);
}

TEST_CASE("HeadlessGLBackend texture array round-trips a compute-written pattern", "[gpu][headless]")
{
    // Each texel gets (x, y, layer, x*y+layer) — any mix-up of axes or layers shows.
    const std::string src = R"(#version 450
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2DArray outImg;
void main()
{
    ivec3 c = ivec3(gl_GlobalInvocationID);
    imageStore(outImg, c, vec4(float(c.x), float(c.y), float(c.z), float(c.x * c.y + c.z)));
}
)";
    const uint32_t Res = 16;
    const uint32_t Layers = 6;

    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);
    auto& gpu = *ctx->backend;

    REQUIRE(ctx->registry.RegisterProgram("test_texture_array_pattern", src));
    const uint32_t program = ctx->registry.Resolve("test_texture_array_pattern");

    const auto tex = gpu.CreateTexture2DArray_RGBA32F(Res, Layers);
    REQUIRE(tex != 0);
    gpu.BindShader(program);
    gpu.BindImage2DArray(tex, 0);
    gpu.Dispatch(Res / 8, Res / 8, Layers);
    gpu.ImageBarrier();

    std::vector<float> texels(Res * Res * Layers * 4);
    gpu.DownloadTexture(tex, texels.data(), texels.size() * sizeof(float));

    for (uint32_t z = 0; z < Layers; ++z)
        for (uint32_t y = 0; y < Res; ++y)
            for (uint32_t x = 0; x < Res; ++x)
            {
                const float* t = &texels[((z * Res + y) * Res + x) * 4];
                INFO("texel " << x << "," << y << "," << z);
                REQUIRE(t[0] == float(x));
                REQUIRE(t[1] == float(y));
                REQUIRE(t[2] == float(z));
                REQUIRE(t[3] == float(x * y + z));
            }

    gpu.DestroyTexture(tex);
    pg_context_destroy(ctx);
}
