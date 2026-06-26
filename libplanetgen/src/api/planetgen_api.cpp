#include "planetgen/planetgen.h"
#include "InternalTypes.h"
#include "../backend/opengl/WindowedGLBackend.h"
#include "../backend/opengl/HeadlessGLBackend.h"
#include "../backend/ParamBlocks.h"
#include "EmbeddedShaders.h"
#include "../model/BodyConfig.h"
#include "../model/BodyConfigProjection.h"
#include <iostream>
#include <fstream>
#include <cstring>

// nlohmann/json for body_create_from_json
#include <nlohmann/json.hpp>

namespace
{

constexpr uint32_t HeightWorkgroupSize  = 512;
constexpr uint32_t ShadingWorkgroupSize = 256;
constexpr uint32_t ErosionWorkgroupSize = 256;

// UBO binding point for param structs — SSBOs use 0/1/2
constexpr uint32_t ParamBinding = 3;

uint32_t ComputeGroupCount(uint32_t count, uint32_t workgroupSize)
{
    return (count + workgroupSize - 1) / workgroupSize;
}

// Fill HeightParams from the body descriptor and call-site seed/count
planetgen::HeightParams MakeHeightParams(const PgBodyDesc& desc, uint32_t seed, uint32_t vertexCount)
{
    planetgen::HeightParams p{};
    p.numVertices = vertexCount;
    p.seed = seed;
    p.useTectonics = desc.use_tectonics;
    p.numPlates = desc.num_plates;
    p.continentalFraction = desc.continental_fraction;
    p.boundaryWidth = desc.boundary_width;
    p.convergentMountainScale = desc.convergent_mountain_scale;
    p.divergentRiftDepth = desc.divergent_rift_depth;
    p.coastlineNoise = desc.coastline_noise;
    p.plateElevationNoise = desc.plate_elevation_noise;
    p.oceanDepthMultiplier = desc.ocean_depth_multiplier;
    p.oceanFloorDepth = desc.ocean_floor_depth;
    p.oceanFloorSmoothing = desc.ocean_floor_smoothing;
    p.mountainBlend = desc.mountain_blend;
    p.heightScale = desc.height_scale;
    p.continentBaseLevel = desc.continent_base_level;
    p.globalFrequency = desc.global_frequency;
    p.normalEpsilon = desc.normal_epsilon;
    p.mountainPower = desc.mountain_power;
    p.mountainGain = desc.mountain_gain;
    p.mountainSmooth = desc.mountain_smoothing;
    p.continentLayers = desc.continent_noise.octaves;
    p.continentScale = desc.continent_noise.scale;
    p.continentPersistence = desc.continent_noise.persistence;
    p.continentLacunarity = desc.continent_noise.lacunarity;
    p.continentMultiplier = desc.continent_noise.strength;
    p.mountainLayers = desc.mountain_noise.octaves;
    p.mountainScale = desc.mountain_noise.scale;
    p.mountainPersistence = desc.mountain_noise.persistence;
    p.mountainLacunarity = desc.mountain_noise.lacunarity;
    p.mountainMultiplier = desc.mountain_noise.strength;
    p.maskLayers = desc.mask_noise.octaves;
    p.maskScale = desc.mask_noise.scale;
    p.maskPersistence = desc.mask_noise.persistence;
    p.maskLacunarity = desc.mask_noise.lacunarity;
    p.maskMultiplier = 1.0f; // always 1 (was hardcoded in old SetHeightUniforms)
    // Offsets are fixed constants from the old implementation
    p.continentOffsetX = 0.0f;    p.continentOffsetY = 0.0f;    p.continentOffsetZ = 0.0f;
    p.mountainOffsetX  = 1000.0f; p.mountainOffsetY  = 1000.0f; p.mountainOffsetZ  = 1000.0f;
    p.maskOffsetX      = 2000.0f; p.maskOffsetY      = 2000.0f; p.maskOffsetZ      = 2000.0f;
    p.detailLowThreshold  = desc.detail_low_threshold;
    p.detailHighThreshold = desc.detail_high_threshold;
    p.perturbStrengthLow  = desc.perturb_strength_low;
    p.perturbStrengthHigh = desc.perturb_strength_high;
    p.detailOctavesLow    = desc.detail_octaves_low;
    p.detailOctavesHigh   = desc.detail_octaves_high;
    p.detailPersistence   = desc.detail_persistence;
    p.detailLacunarity    = desc.detail_lacunarity;
    p.perturbScale        = desc.perturb_scale;
    p.useOceanFloor       = desc.use_ocean_floor;
    p.shelfWidth          = desc.shelf_width;
    p.oceanRidgeOctaves   = desc.ocean_ridge_octaves;
    p.oceanRidgeScale     = desc.ocean_ridge_scale;
    p.oceanRidgeStrength  = desc.ocean_ridge_strength;
    p.oceanRidgePower     = desc.ocean_ridge_power;
    p.oceanRidgeGain      = desc.ocean_ridge_gain;
    p.trenchOctaves       = desc.trench_octaves;
    p.trenchScale         = desc.trench_scale;
    p.trenchDepth         = desc.trench_depth;
    p.abyssalOctaves      = desc.abyssal_octaves;
    p.abyssalScale        = desc.abyssal_scale;
    p.abyssalStrength     = desc.abyssal_strength;
    return p;
}

planetgen::ShadingEarthParams MakeShadingEarthParams(const PgShadingDesc& desc, uint32_t seed, uint32_t vertexCount)
{
    planetgen::ShadingEarthParams p{};
    p.numVertices            = vertexCount;
    p.seed                   = seed;
    p.useClimateModel        = desc.use_climate_model;
    p.largeNoiseOctaves      = desc.large_noise_octaves;
    p.largeNoiseScale        = desc.large_noise_scale;
    p.detailNoiseScale       = desc.detail_noise_scale;
    p.smallNoiseScale        = desc.small_noise_scale;
    p.smallNoiseOctaves      = desc.small_noise_octaves;
    p.warpStrength           = desc.warp_strength;
    p.temperatureLapseRate   = desc.temperature_lapse_rate;
    p.temperatureExponent    = desc.temperature_exponent;
    p.moistureNoiseScale     = desc.moisture_noise_scale;
    p.moistureNoiseStrength  = desc.moisture_noise_strength;
    p.hadleyIntensity        = desc.hadley_intensity;
    p.continentalityStrength = desc.continentality_strength;
    p.heightScale            = desc.height_scale;
    return p;
}

planetgen::ShadingGenericParams MakeShadingGenericParams(const PgShadingDesc& desc, uint32_t seed, uint32_t vertexCount)
{
    planetgen::ShadingGenericParams p{};
    p.numVertices      = vertexCount;
    p.seed             = seed;
    p.heightScale      = desc.height_scale;
    p.detailNoiseScale = desc.detail_noise_scale;
    p.smallNoiseScale  = desc.small_noise_scale;
    p.smallNoiseOctaves = desc.small_noise_octaves;
    p.warpStrength     = desc.warp_strength;
    return p;
}

planetgen::ErosionParams MakeErosionParams(const PgErosionDesc& desc, uint32_t vertexCount)
{
    planetgen::ErosionParams p{};
    p.numVertices     = vertexCount;
    p.gridResolution  = desc.grid_resolution;
    p.thermalRate     = desc.thermal_rate;
    p.thermalThreshold = desc.thermal_threshold;
    p.hydraulicRate   = desc.hydraulic_rate;
    p.depositionRate  = desc.deposition_rate;
    p.evaporationRate = desc.evaporation_rate;
    return p;
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

    uint32_t program = ctx->registry.Resolve(planetgen::BuiltinProgram::Height);
    if (!program)
    {
        ctx->lastError = PG_ERROR_NOT_INITIALIZED;
        return nullptr;
    }

    size_t vertexBytes = vertex_count * 3 * sizeof(float);
    size_t heightBytes = vertex_count * sizeof(float);
    size_t normalBytes = vertex_count * 3 * sizeof(float);

    auto vertexBuf = gpu.CreateBuffer(vertexBytes);
    auto heightBuf = gpu.CreateBuffer(heightBytes);
    auto normalBuf = gpu.CreateBuffer(normalBytes);

    gpu.UploadBuffer(vertexBuf, vertices, vertexBytes);

    gpu.BindShader(program);
    gpu.BindBuffer(vertexBuf, 0);
    gpu.BindBuffer(heightBuf, 1);
    gpu.BindBuffer(normalBuf, 2);

    // Upload typed params via UBO
    auto params = MakeHeightParams(body->desc, seed, vertex_count);
    auto ubo = gpu.CreateParamBuffer(sizeof(params));
    gpu.SetParams(ubo, &params, sizeof(params), ParamBinding);

    gpu.Dispatch(ComputeGroupCount(vertex_count, HeightWorkgroupSize));
    gpu.Barrier();

    gpu.DestroyParamBuffer(ubo);

    auto result = new (std::nothrow) PgResult_T();
    if (!result)
    {
        gpu.DestroyBuffer(vertexBuf);
        gpu.DestroyBuffer(heightBuf);
        gpu.DestroyBuffer(normalBuf);
        return nullptr;
    }

    result->type = PgResultType::Heights;
    result->count = vertex_count;
    result->heights.resize(vertex_count);
    result->normals.resize(vertex_count * 3);

    gpu.DownloadBuffer(heightBuf, result->heights.data(), heightBytes);
    gpu.DownloadBuffer(normalBuf, result->normals.data(), normalBytes);

    result->heightsGpuBuffer = heightBuf;
    result->normalsGpuBuffer = normalBuf;

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

    // Choose between climate and generic shader based on descriptor
    using BP = planetgen::BuiltinProgram;
    BP programId = desc->use_climate_model ? BP::ShadingEarth : BP::ShadingGeneric;
    uint32_t program = ctx->registry.Resolve(programId);
    if (!program)
    {
        // Fall back to whichever is available
        program = ctx->registry.Resolve(BP::ShadingEarth);
        if (!program) program = ctx->registry.Resolve(BP::ShadingGeneric);
        if (!program)
        {
            ctx->lastError = PG_ERROR_NOT_INITIALIZED;
            return nullptr;
        }
    }

    size_t vertexBytes  = vertex_count * 3 * sizeof(float);
    size_t heightBytes  = vertex_count * sizeof(float);
    size_t shadingBytes = vertex_count * 4 * sizeof(float);

    auto vertexBuf  = gpu.CreateBuffer(vertexBytes);
    auto shadingBuf = gpu.CreateBuffer(shadingBytes);
    auto heightBuf  = gpu.CreateBuffer(heightBytes);

    gpu.UploadBuffer(vertexBuf, vertices, vertexBytes);
    gpu.UploadBuffer(heightBuf, heights,  heightBytes);

    gpu.BindShader(program);
    gpu.BindBuffer(vertexBuf,  0);
    gpu.BindBuffer(shadingBuf, 1);
    gpu.BindBuffer(heightBuf,  2);

    // Upload the correct param struct for whichever path we took
    planetgen::GpuBufferHandle ubo = 0;
    if (programId == BP::ShadingEarth)
    {
        auto p = MakeShadingEarthParams(*desc, seed, vertex_count);
        ubo = gpu.CreateParamBuffer(sizeof(p));
        gpu.SetParams(ubo, &p, sizeof(p), ParamBinding);
    }
    else
    {
        auto p = MakeShadingGenericParams(*desc, seed, vertex_count);
        ubo = gpu.CreateParamBuffer(sizeof(p));
        gpu.SetParams(ubo, &p, sizeof(p), ParamBinding);
    }

    gpu.Dispatch(ComputeGroupCount(vertex_count, ShadingWorkgroupSize));
    gpu.Barrier();

    gpu.DestroyParamBuffer(ubo);

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
                              const PgErosionDesc* desc, uint32_t /*seed*/)
{
    if (!body || !heights || vertex_count == 0 || !desc)
        return nullptr;

    auto* ctx = body->ctx;
    auto& gpu = *ctx->backend;

    uint32_t program = ctx->registry.Resolve(planetgen::BuiltinProgram::Erosion);
    if (!program)
    {
        ctx->lastError = PG_ERROR_NOT_INITIALIZED;
        return nullptr;
    }

    size_t heightBytes = vertex_count * sizeof(float);

    auto inputBuf  = gpu.CreateBuffer(heightBytes);
    auto outputBuf = gpu.CreateBuffer(heightBytes);

    gpu.UploadBuffer(inputBuf, heights, heightBytes);
    gpu.BindShader(program);

    // Params are the same every iteration — create once, re-upload each iter
    auto params = MakeErosionParams(*desc, vertex_count);
    auto ubo = gpu.CreateParamBuffer(sizeof(params));

    for (int i = 0; i < desc->iterations; ++i)
    {
        gpu.BindBuffer(inputBuf,  0);
        gpu.BindBuffer(outputBuf, 1);

        gpu.SetParams(ubo, &params, sizeof(params), ParamBinding);

        gpu.Dispatch(ComputeGroupCount(vertex_count, ErosionWorkgroupSize));
        gpu.Barrier();

        // Ping-pong
        auto tmp = inputBuf;
        inputBuf  = outputBuf;
        outputBuf = tmp;
    }

    gpu.DestroyParamBuffer(ubo);

    auto result = new (std::nothrow) PgResult_T();
    if (!result)
    {
        gpu.DestroyBuffer(inputBuf);
        gpu.DestroyBuffer(outputBuf);
        return nullptr;
    }

    result->type = PgResultType::Erosion;
    result->count = vertex_count;
    result->heights.resize(vertex_count);

    gpu.DownloadBuffer(inputBuf, result->heights.data(), heightBytes);

    result->heightsGpuBuffer = inputBuf;
    gpu.DestroyBuffer(outputBuf);

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
