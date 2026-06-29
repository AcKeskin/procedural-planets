#include "planetgen/planetgen.h"
#include "InternalTypes.h"
#include "../backend/opengl/WindowedGLBackend.h"
#include "../backend/opengl/HeadlessGLBackend.h"
#include "EmbeddedShaders.h"
#include "../model/BodyConfig.h"
#include "../model/BodyConfigProjection.h"
#include "../model/BodyConfigJson.h"
#include "../strategy/HeightStrategy.h"
#include "../strategy/ShadingStrategy.h"
#include "../strategy/ErosionStrategy.h"
#include "../strategy/GenerationPipeline.h"
#include "../mesh/MeshGen.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <exception>
#include <new>
#include <string>

// Dispatch mechanics (param mapping, workgroup sizing, GPU buffer threading)
// now live in the strategy layer (src/strategy/*). This file owns only the
// C ABI surface: handle lifecycle, result construction, and CPU readback.

namespace
{

// Resolve the body's palette by id and copy it into the result (FFI-safe entries).
void FillPalette(PgResult_T* result, const PgBody_T* body)
{
    const planetgen::Palette& pal =
        body->ctx->palettes.Resolve(body->config.paletteRef.paletteId);
    result->palette.clear();
    result->palette.reserve(pal.entries.size());
    for (const auto& e : pal.entries)
    {
        PgPaletteEntry out{};
        out.color[0] = e.color.x;
        out.color[1] = e.color.y;
        out.color[2] = e.color.z;
        out.parameter = e.parameter;
        result->palette.push_back(out);
    }
}

// Run a generation body with an exception boundary so nothing crosses the C ABI.
// On a thrown exception, record code+message on the context and return null.
template <typename Fn>
PgResult GuardGen(PgContext_T* ctx, Fn&& fn)
{
    try
    {
        return fn();
    }
    catch (const std::bad_alloc&)
    {
        if (ctx) ctx->SetError(PG_ERROR_OUT_OF_MEMORY, "out of memory during generation");
        return nullptr;
    }
    catch (const std::exception& e)
    {
        if (ctx) ctx->SetError(PG_ERROR_INVALID_ARGUMENT,
                               std::string("generation failed: ") + e.what());
        return nullptr;
    }
    catch (...)
    {
        if (ctx) ctx->SetError(PG_ERROR_INVALID_ARGUMENT, "generation failed: unknown error");
        return nullptr;
    }
}

} // namespace

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
            ctx->SetError(PG_ERROR_SHADER_COMPILE_FAILED,
                          std::string("failed to compile builtin shader: ") + label);
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

const char* pg_get_last_error_message(PgContext ctx)
{
    if (!ctx)
        return "null context";
    return ctx->lastErrorMessage.c_str();
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
    body->desc = *desc; // flat path: desc authoritative, config stays default
    ctx->ClearError();
    return body;
}

PgBody pg_body_create_from_json(PgContext ctx, const char* json_path)
{
    if (!ctx || !json_path)
        return nullptr;

    // JSON is the canonical composable path: load the full typed BodyConfig (all
    // strategy-aligned blocks, with per-block defaults + validation), then project
    // it down to the flat desc the strategies bind. Earth is just one such config.
    planetgen::BodyConfig config;
    const std::string err = planetgen::BodyConfigLoadFile(json_path, config);
    if (!err.empty())
    {
        // Distinguish "file missing" from "bad contents" for the status code.
        std::ifstream probe(json_path);
        const PgError code = probe.is_open() ? PG_ERROR_JSON_PARSE_FAILED
                                             : PG_ERROR_FILE_NOT_FOUND;
        ctx->SetError(code, std::string("body config load failed: ") + err);
        return nullptr;
    }

    auto body = new (std::nothrow) PgBody_T();
    if (!body)
    {
        ctx->SetError(PG_ERROR_OUT_OF_MEMORY, "out of memory creating body");
        return nullptr;
    }

    body->ctx = ctx;
    body->config = config;
    body->desc = planetgen::ProjectToBodyDesc(config);
    ctx->ClearError();
    return body;
}

PgBody pg_body_create_from_json_string(PgContext ctx, const char* json)
{
    if (!ctx || !json)
        return nullptr;

    // Same canonical composable path as the file variant, but the caller supplies
    // the JSON in memory (e.g. an app's in-flight, GUI-edited config) — config as
    // data, no filesystem round-trip.
    planetgen::BodyConfig config;
    const std::string err = planetgen::BodyConfigFromJson(json, config);
    if (!err.empty())
    {
        ctx->SetError(PG_ERROR_JSON_PARSE_FAILED,
                      std::string("body config parse failed: ") + err);
        return nullptr;
    }

    auto body = new (std::nothrow) PgBody_T();
    if (!body)
    {
        ctx->SetError(PG_ERROR_OUT_OF_MEMORY, "out of memory creating body");
        return nullptr;
    }

    body->ctx = ctx;
    body->config = config;
    body->desc = planetgen::ProjectToBodyDesc(config);
    ctx->ClearError();
    return body;
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
    return GuardGen(ctx, [&]() -> PgResult {
    auto& gpu = *ctx->backend;

    if (!ctx->registry.Resolve(planetgen::BuiltinProgram::Height))
    {
        ctx->SetError(PG_ERROR_NOT_INITIALIZED, "height program not registered");
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
        ctx->SetError(PG_ERROR_NOT_INITIALIZED, "height strategy failed to produce buffers");
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

    FillPalette(result, body);
    ctx->ClearError();
    return result;
    });
}

PgResult pg_generate_shading(PgBody body, const float* vertices, const float* heights,
                              uint32_t vertex_count, uint32_t seed, const PgShadingDesc* desc)
{
    if (!body || !vertices || !heights || vertex_count == 0 || !desc)
        return nullptr;

    auto* ctx = body->ctx;
    return GuardGen(ctx, [&]() -> PgResult {
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
        ctx->SetError(PG_ERROR_NOT_INITIALIZED, "shading program not registered");
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

    FillPalette(result, body);
    ctx->ClearError();
    return result;
    });
}

PgResult pg_generate_erosion(PgBody body, const float* heights, uint32_t vertex_count,
                              const PgErosionDesc* desc, uint32_t seed)
{
    if (!body || !heights || vertex_count == 0 || !desc)
        return nullptr;

    auto* ctx = body->ctx;
    return GuardGen(ctx, [&]() -> PgResult {
    auto& gpu = *ctx->backend;

    if (!ctx->registry.Resolve(planetgen::BuiltinProgram::Erosion))
    {
        ctx->SetError(PG_ERROR_NOT_INITIALIZED, "erosion program not registered");
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

    FillPalette(result, body);
    ctx->ClearError();
    return result;
    });
}

PgResult pg_generate_mesh(PgBody body, uint32_t rings, uint32_t segments, uint32_t seed)
{
    if (!body)
        return nullptr;

    auto* ctx = body->ctx;
    return GuardGen(ctx, [&]() -> PgResult {

    // Build geometry, then run the full pipeline over it (height -> [erosion] ->
    // shading + palette). One call gives the consumer mesh + per-vertex data.
    planetgen::MeshData mesh = planetgen::MakeUvSphere(rings, segments);
    const uint32_t vertexCount = static_cast<uint32_t>(mesh.positions.size() / 3);

    planetgen::GenerationPipeline pipeline(*ctx->backend, ctx->registry, ctx->palettes);
    const planetgen::GenerationPipeline::MaskCache maskCache{ &body->maskTexture, &body->maskBuiltKey };
    planetgen::PipelineResult pr =
        pipeline.Generate(body->config, mesh.positions.data(), vertexCount, seed, maskCache);

    if (pr.count == 0)
    {
        ctx->SetError(PG_ERROR_NOT_INITIALIZED, "mesh generation pipeline produced no data");
        return nullptr;
    }

    auto result = new (std::nothrow) PgResult_T();
    if (!result)
    {
        ctx->SetError(PG_ERROR_OUT_OF_MEMORY, "out of memory creating mesh result");
        return nullptr;
    }

    result->type = PgResultType::Heights;
    result->count = pr.count;
    result->heights = std::move(pr.heights);
    result->normals = std::move(pr.normals);
    result->shading = std::move(pr.shading);
    result->vertices = std::move(mesh.positions);
    result->indices  = std::move(mesh.indices);
    result->heightsGpuBuffer = pr.heightsGpuBuffer;
    result->normalsGpuBuffer = pr.normalsGpuBuffer;
    result->shadingGpuBuffer = pr.shadingGpuBuffer;

    FillPalette(result, body);
    ctx->ClearError();
    return result;
    });
}

PgError pg_generate_patch(PgBody body, const float* vertices, uint32_t vertex_count, uint32_t seed,
                          uint32_t height_buffer, uint32_t normal_buffer, uint32_t shading_buffer,
                          uint32_t* out_vertex_count)
{
    if (!body || !vertices || vertex_count == 0 || !height_buffer || !normal_buffer || !shading_buffer)
        return PG_ERROR_INVALID_ARGUMENT;

    auto* ctx = body->ctx;

    // Status-returning exception boundary (the result-returning GuardGen doesn't
    // fit — this path allocates no PgResult). Nothing crosses the C ABI.
    try
    {
        planetgen::GenerationPipeline pipeline(*ctx->backend, ctx->registry, ctx->palettes);
        const planetgen::GenerationPipeline::MaskCache maskCache{ &body->maskTexture, &body->maskBuiltKey };
        const bool ok = pipeline.GeneratePatch(body->config, vertices, vertex_count, seed,
                                               height_buffer, normal_buffer, shading_buffer, maskCache);
        if (!ok)
        {
            ctx->SetError(PG_ERROR_NOT_INITIALIZED, "per-patch generation produced no dispatch");
            return ctx->lastError;
        }

        if (out_vertex_count)
            *out_vertex_count = vertex_count;

        ctx->ClearError();
        return PG_SUCCESS;
    }
    catch (const std::bad_alloc&)
    {
        ctx->SetError(PG_ERROR_OUT_OF_MEMORY, "out of memory during per-patch generation");
        return ctx->lastError;
    }
    catch (const std::exception& e)
    {
        ctx->SetError(PG_ERROR_INVALID_ARGUMENT,
                      std::string("per-patch generation failed: ") + e.what());
        return ctx->lastError;
    }
    catch (...)
    {
        ctx->SetError(PG_ERROR_INVALID_ARGUMENT, "per-patch generation failed: unknown error");
        return ctx->lastError;
    }
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

const PgPaletteEntry* pg_result_get_palette(PgResult result)
{
    if (!result || result->palette.empty())
        return nullptr;
    return result->palette.data();
}

uint32_t pg_result_get_palette_count(PgResult result)
{
    if (!result)
        return 0;
    return static_cast<uint32_t>(result->palette.size());
}

const float* pg_result_get_vertices(PgResult result)
{
    if (!result || result->vertices.empty())
        return nullptr;
    return result->vertices.data();
}

const uint32_t* pg_result_get_indices(PgResult result)
{
    if (!result || result->indices.empty())
        return nullptr;
    return result->indices.data();
}

uint32_t pg_result_get_index_count(PgResult result)
{
    if (!result)
        return 0;
    return static_cast<uint32_t>(result->indices.size());
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
