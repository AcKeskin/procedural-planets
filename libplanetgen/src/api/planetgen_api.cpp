#include "planetgen/planetgen.h"
#include "InternalTypes.h"
#include "../backend/opengl/WindowedGLBackend.h"
#include "../backend/opengl/HeadlessGLBackend.h"
#include "EmbeddedShaders.h"
#include "../model/BodyConfig.h"
#include "../model/BodyConfigProjection.h"
#include "../strategy/HeightStrategy.h"
#include "../strategy/ShadingStrategy.h"
#include "../strategy/ErosionStrategy.h"
#include <iostream>
#include <fstream>
#include <cstring>

// nlohmann/json for body_create_from_json
#include <nlohmann/json.hpp>

// Dispatch mechanics (param mapping, workgroup sizing, GPU buffer threading)
// now live in the strategy layer (src/strategy/*). This file owns only the
// C ABI surface: handle lifecycle, result construction, and CPU readback.

// ============================================================================
// Context lifecycle
// ============================================================================

PgContext pg_context_create(const PgContextDesc* desc)
{
    if (!desc)
        return nullptr;

    auto ctx = new (std::nothrow) PgContext_T();
    if (!ctx)
        return nullptr;

    std::unique_ptr<planetgen::IComputeBackend> backend;
    bool ok = false;

    if (desc->use_existing_context)
    {
        auto windowed = std::make_unique<planetgen::WindowedGLBackend>();
        ok = windowed->Init();
        backend = std::move(windowed);
    }
    else
    {
        auto headless = std::make_unique<planetgen::HeadlessGLBackend>();
        ok = headless->Init();
        backend = std::move(headless);
    }

    if (!ok)
    {
        delete ctx;
        return nullptr;
    }

    ctx->backend = std::move(backend);

    // Wire the registry to the backend's compile/destroy routines
    auto* backendPtr = ctx->backend.get();
    ctx->registry.SetCompiler(
        [backendPtr](const std::string& src) { return backendPtr->CompileShader(src); },
        [backendPtr](uint32_t prog)           { backendPtr->DestroyShader(prog); }
    );

    // Compile and register all four built-in shaders
    using BP = planetgen::BuiltinProgram;
    auto reg = [&](BP id, const std::string& src, const char* label) -> bool
    {
        if (!ctx->registry.RegisterProgram(id, src))
        {
            ctx->lastError = PG_ERROR_SHADER_COMPILE_FAILED;
            std::cerr << "[planetgen] Failed to compile " << label << " shader" << std::endl;
            return false;
        }
        return true;
    };

    reg(BP::Height,          planetgen::shaders::height_earth_comp,       "height");
    reg(BP::ShadingEarth,    planetgen::shaders::shading_earth_comp,      "shading_earth");
    reg(BP::ShadingGeneric,  planetgen::shaders::shading_generic_comp,    "shading_generic");
    reg(BP::Erosion,         planetgen::shaders::erosion_earth_comp,      "erosion");
    reg(BP::ContinentGrowth, planetgen::shaders::continent_growth_comp,   "continent_growth");

    return ctx;
}

void pg_context_destroy(PgContext ctx)
{
    if (!ctx)
        return;

    // Registry destructor fires first, calls DestroyShader for each program.
    // Then backend destructor tears down the GL context.
    delete ctx;
}

PgError pg_context_get_error(PgContext ctx)
{
    if (!ctx)
        return PG_ERROR_INVALID_ARGUMENT;
    return ctx->lastError;
}

// ============================================================================
// Body configuration
// ============================================================================

PgBody pg_body_create(PgContext ctx, const PgBodyDesc* desc)
{
    if (!ctx || !desc)
        return nullptr;

    auto body = new (std::nothrow) PgBody_T();
    if (!body)
        return nullptr;

    body->ctx = ctx;
    body->desc = *desc;
    return body;
}

PgBody pg_body_create_from_json(PgContext ctx, const char* json_path)
{
    if (!ctx || !json_path)
        return nullptr;

    std::ifstream file(json_path);
    if (!file.is_open())
    {
        ctx->lastError = PG_ERROR_FILE_NOT_FOUND;
        return nullptr;
    }

    try
    {
        nlohmann::json json;
        file >> json;

        PgBodyDesc desc{};
        pg_body_desc_defaults(&desc);

        auto parseNoise = [](const nlohmann::json& j, PgNoiseLayer& layer)
        {
            if (j.contains("scale")) layer.scale = j["scale"].get<float>();
            if (j.contains("octaves")) layer.octaves = j["octaves"].get<int>();
            if (j.contains("persistence")) layer.persistence = j["persistence"].get<float>();
            if (j.contains("lacunarity")) layer.lacunarity = j["lacunarity"].get<float>();
            if (j.contains("strength")) layer.strength = j["strength"].get<float>();
        };

        if (json.contains("terrain"))
        {
            auto& t = json["terrain"];
            desc.height_scale = t.value("heightScale", desc.height_scale);
            desc.ocean_depth_multiplier = t.value("oceanDepthMultiplier", desc.ocean_depth_multiplier);
            desc.ocean_floor_depth = t.value("oceanFloorDepth", desc.ocean_floor_depth);
            desc.mountain_blend = t.value("mountainBlend", desc.mountain_blend);

            if (t.contains("continent")) parseNoise(t["continent"], desc.continent_noise);
            if (t.contains("mountain")) parseNoise(t["mountain"], desc.mountain_noise);
            if (t.contains("mask")) parseNoise(t["mask"], desc.mask_noise);
        }

        return pg_body_create(ctx, &desc);
    }
    catch (const nlohmann::json::exception& e)
    {
        std::cerr << "[planetgen] JSON parse error: " << e.what() << std::endl;
        ctx->lastError = PG_ERROR_JSON_PARSE_FAILED;
        return nullptr;
    }
}

void pg_body_destroy(PgBody body)
{
    delete body;
}

// ============================================================================
// Generation dispatch
// ============================================================================

PgResult pg_generate_heights(PgBody body, const float* vertices, uint32_t vertex_count, uint32_t seed)
{
    if (!body || !vertices || vertex_count == 0)
        return nullptr;

    auto* ctx = body->ctx;
    auto& gpu = *ctx->backend;

    if (!ctx->registry.Resolve(planetgen::BuiltinProgram::Height))
    {
        ctx->lastError = PG_ERROR_NOT_INITIALIZED;
        return nullptr;
    }

    const size_t vertexBytes = vertex_count * 3 * sizeof(float);
    const size_t heightBytes = vertex_count * sizeof(float);
    const size_t normalBytes = vertex_count * 3 * sizeof(float);

    const auto vertexBuf = gpu.CreateBuffer(vertexBytes);
    gpu.UploadBuffer(vertexBuf, vertices, vertexBytes);

    // Dispatch through the height strategy — the C ABI keeps per-call granularity.
    planetgen::StrategyContext sc{ gpu, ctx->registry, vertex_count, seed };
    planetgen::HeightStrategy strategy;
    const planetgen::HeightBuffers hb = strategy.Run(sc, body->desc, vertexBuf);

    if (!hb.heights || !hb.normals)
    {
        gpu.DestroyBuffer(vertexBuf);
        if (hb.heights) gpu.DestroyBuffer(hb.heights);
        if (hb.normals) gpu.DestroyBuffer(hb.normals);
        ctx->lastError = PG_ERROR_NOT_INITIALIZED;
        return nullptr;
    }

    auto result = new (std::nothrow) PgResult_T();
    if (!result)
    {
        gpu.DestroyBuffer(vertexBuf);
        gpu.DestroyBuffer(hb.heights);
        gpu.DestroyBuffer(hb.normals);
        return nullptr;
    }

    result->type = PgResultType::Heights;
    result->count = vertex_count;
    result->heights.resize(vertex_count);
    result->normals.resize(vertex_count * 3);

    gpu.DownloadBuffer(hb.heights, result->heights.data(), heightBytes);
    gpu.DownloadBuffer(hb.normals, result->normals.data(), normalBytes);

    result->heightsGpuBuffer = hb.heights;
    result->normalsGpuBuffer = hb.normals;

    gpu.DestroyBuffer(vertexBuf);

    return result;
}

PgResult pg_generate_shading(PgBody body, const float* vertices, const float* heights,
                              uint32_t vertex_count, uint32_t seed, const PgShadingDesc* desc)
{
    if (!body || !vertices || !heights || vertex_count == 0 || !desc)
        return nullptr;

    auto* ctx = body->ctx;
    auto& gpu = *ctx->backend;

    const size_t vertexBytes  = vertex_count * 3 * sizeof(float);
    const size_t heightBytes  = vertex_count * sizeof(float);
    const size_t shadingBytes = vertex_count * 4 * sizeof(float);

    const auto vertexBuf = gpu.CreateBuffer(vertexBytes);
    const auto heightBuf = gpu.CreateBuffer(heightBytes);
    gpu.UploadBuffer(vertexBuf, vertices, vertexBytes);
    gpu.UploadBuffer(heightBuf, heights,  heightBytes);

    // Dispatch through the shading strategy — earth/generic choice lives inside it.
    planetgen::StrategyContext sc{ gpu, ctx->registry, vertex_count, seed };
    planetgen::ShadingStrategy strategy;
    const planetgen::GpuBufferHandle shadingBuf =
        strategy.Run(sc, *desc, vertexBuf, heightBuf);

    if (!shadingBuf)
    {
        gpu.DestroyBuffer(vertexBuf);
        gpu.DestroyBuffer(heightBuf);
        ctx->lastError = PG_ERROR_NOT_INITIALIZED;
        return nullptr;
    }

    auto result = new (std::nothrow) PgResult_T();
    if (!result)
    {
        gpu.DestroyBuffer(vertexBuf);
        gpu.DestroyBuffer(shadingBuf);
        gpu.DestroyBuffer(heightBuf);
        return nullptr;
    }

    result->type = PgResultType::Shading;
    result->count = vertex_count;
    result->shading.resize(vertex_count * 4);

    gpu.DownloadBuffer(shadingBuf, result->shading.data(), shadingBytes);

    result->shadingGpuBuffer = shadingBuf;

    gpu.DestroyBuffer(vertexBuf);
    gpu.DestroyBuffer(heightBuf);

    return result;
}

PgResult pg_generate_erosion(PgBody body, const float* heights, uint32_t vertex_count,
                              const PgErosionDesc* desc, uint32_t seed)
{
    if (!body || !heights || vertex_count == 0 || !desc)
        return nullptr;

    auto* ctx = body->ctx;
    auto& gpu = *ctx->backend;

    if (!ctx->registry.Resolve(planetgen::BuiltinProgram::Erosion))
    {
        ctx->lastError = PG_ERROR_NOT_INITIALIZED;
        return nullptr;
    }

    const size_t heightBytes = vertex_count * sizeof(float);

    const auto inputBuf = gpu.CreateBuffer(heightBytes);
    gpu.UploadBuffer(inputBuf, heights, heightBytes);

    // Dispatch through the erosion strategy — it owns the ping-pong + scratch.
    planetgen::StrategyContext sc{ gpu, ctx->registry, vertex_count, seed };
    planetgen::ErosionStrategy strategy;
    const planetgen::GpuBufferHandle resultBuf = strategy.Run(sc, *desc, inputBuf);

    auto result = new (std::nothrow) PgResult_T();
    if (!result)
    {
        gpu.DestroyBuffer(inputBuf);
        return nullptr;
    }

    result->type = PgResultType::Erosion;
    result->count = vertex_count;
    result->heights.resize(vertex_count);

    gpu.DownloadBuffer(resultBuf, result->heights.data(), heightBytes);

    result->heightsGpuBuffer = resultBuf;

    // The strategy may have returned the input or its own scratch; free the other.
    if (resultBuf != inputBuf)
        gpu.DestroyBuffer(inputBuf);

    return result;
}

// ============================================================================
// Result access
// ============================================================================

const float* pg_result_get_heights(PgResult result)
{
    if (!result || result->heights.empty())
        return nullptr;
    return result->heights.data();
}

const float* pg_result_get_normals(PgResult result)
{
    if (!result || result->normals.empty())
        return nullptr;
    return result->normals.data();
}

const float* pg_result_get_shading(PgResult result)
{
    if (!result || result->shading.empty())
        return nullptr;
    return result->shading.data();
}

uint32_t pg_result_get_count(PgResult result)
{
    if (!result)
        return 0;
    return result->count;
}

uint32_t pg_result_get_gpu_buffer(PgResult result)
{
    if (!result)
        return 0;

    switch (result->type)
    {
    case PgResultType::Heights:
    case PgResultType::Erosion:
        return result->heightsGpuBuffer;
    case PgResultType::Shading:
        return result->shadingGpuBuffer;
    }
    return 0;
}

void pg_result_destroy(PgResult result)
{
    // GPU buffers are not destroyed here — they may still be in use by the consumer.
    delete result;
}

// ============================================================================
// Utility — default descriptors
// ============================================================================

// Defaults now delegate to the typed BodyConfig model — single source of truth.
void pg_body_desc_defaults(PgBodyDesc* desc)
{
    if (!desc)
        return;
    *desc = planetgen::ProjectToBodyDesc(planetgen::BodyConfig{});
}

void pg_shading_desc_defaults(PgShadingDesc* desc)
{
    if (!desc)
        return;
    *desc = planetgen::ProjectToShadingDesc(planetgen::BodyConfig{});
}

void pg_erosion_desc_defaults(PgErosionDesc* desc)
{
    if (!desc)
        return;
    *desc = planetgen::ProjectToErosionDesc(planetgen::BodyConfig{});
}
