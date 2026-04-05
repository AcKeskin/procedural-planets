#include "planetgen/planetgen.h"
#include "InternalTypes.h"
#include "../backend/opengl/OpenGLBackend.h"
#include "EmbeddedShaders.h"
#include <iostream>
#include <fstream>
#include <cstring>

// nlohmann/json for body_create_from_json
#include <nlohmann/json.hpp>

namespace
{

constexpr uint32_t HeightWorkgroupSize = 512;
constexpr uint32_t ShadingWorkgroupSize = 256;
constexpr uint32_t ErosionWorkgroupSize = 256;

uint32_t ComputeGroupCount(uint32_t count, uint32_t workgroupSize)
{
    return (count + workgroupSize - 1) / workgroupSize;
}

// Set height generation uniforms on the backend
void SetHeightUniforms(planetgen::IComputeBackend& gpu, const PgBodyDesc& desc, uint32_t seed, uint32_t vertexCount)
{
    gpu.SetUint("numVertices", vertexCount);
    gpu.SetUint("seed", seed);

    // Continent noise
    const float continentOffset[3] = {0.0f, 0.0f, 0.0f};
    gpu.SetVec3("continentOffset", continentOffset);
    gpu.SetInt("continentLayers", desc.continent_noise.octaves);
    gpu.SetFloat("continentScale", desc.continent_noise.scale);
    gpu.SetFloat("continentPersistence", desc.continent_noise.persistence);
    gpu.SetFloat("continentLacunarity", desc.continent_noise.lacunarity);
    gpu.SetFloat("continentMultiplier", desc.continent_noise.strength);

    // Mountain noise
    const float mountainOffset[3] = {1000.0f, 1000.0f, 1000.0f};
    gpu.SetVec3("mountainOffset", mountainOffset);
    gpu.SetInt("mountainLayers", desc.mountain_noise.octaves);
    gpu.SetFloat("mountainScale", desc.mountain_noise.scale);
    gpu.SetFloat("mountainPersistence", desc.mountain_noise.persistence);
    gpu.SetFloat("mountainLacunarity", desc.mountain_noise.lacunarity);
    gpu.SetFloat("mountainMultiplier", desc.mountain_noise.strength);
    gpu.SetFloat("mountainPower", desc.mountain_power);
    gpu.SetFloat("mountainGain", desc.mountain_gain);
    gpu.SetFloat("mountainSmooth", desc.mountain_smoothing);

    // Mask noise
    const float maskOffset[3] = {2000.0f, 2000.0f, 2000.0f};
    gpu.SetVec3("maskOffset", maskOffset);
    gpu.SetInt("maskLayers", desc.mask_noise.octaves);
    gpu.SetFloat("maskScale", desc.mask_noise.scale);
    gpu.SetFloat("maskPersistence", desc.mask_noise.persistence);
    gpu.SetFloat("maskLacunarity", desc.mask_noise.lacunarity);
    gpu.SetFloat("maskMultiplier", 1.0f);

    // Terrain parameters
    gpu.SetFloat("oceanDepthMultiplier", desc.ocean_depth_multiplier);
    gpu.SetFloat("oceanFloorDepth", desc.ocean_floor_depth);
    gpu.SetFloat("oceanFloorSmoothing", desc.ocean_floor_smoothing);
    gpu.SetFloat("mountainBlend", desc.mountain_blend);
    gpu.SetFloat("heightScale", desc.height_scale);
    gpu.SetFloat("continentBaseLevel", desc.continent_base_level);
    gpu.SetFloat("globalFrequency", desc.global_frequency);
    gpu.SetFloat("normalEpsilon", desc.normal_epsilon);

    // Tectonics
    gpu.SetInt("useTectonics", desc.use_tectonics);
    gpu.SetInt("numPlates", desc.num_plates);
    gpu.SetFloat("continentalFraction", desc.continental_fraction);
    gpu.SetFloat("boundaryWidth", desc.boundary_width);
    gpu.SetFloat("convergentMountainScale", desc.convergent_mountain_scale);
    gpu.SetFloat("divergentRiftDepth", desc.divergent_rift_depth);
    gpu.SetFloat("coastlineNoise", desc.coastline_noise);
    gpu.SetFloat("plateElevationNoise", desc.plate_elevation_noise);

    // Height-dependent detail
    gpu.SetFloat("detailLowThreshold", desc.detail_low_threshold);
    gpu.SetFloat("detailHighThreshold", desc.detail_high_threshold);
    gpu.SetFloat("perturbStrengthLow", desc.perturb_strength_low);
    gpu.SetFloat("perturbStrengthHigh", desc.perturb_strength_high);
    gpu.SetInt("detailOctavesLow", desc.detail_octaves_low);
    gpu.SetInt("detailOctavesHigh", desc.detail_octaves_high);
    gpu.SetFloat("detailPersistence", desc.detail_persistence);
    gpu.SetFloat("detailLacunarity", desc.detail_lacunarity);
    gpu.SetFloat("perturbScale", desc.perturb_scale);

    // Ocean floor topology
    gpu.SetInt("useOceanFloor", desc.use_ocean_floor);
    gpu.SetFloat("shelfWidth", desc.shelf_width);
    gpu.SetInt("oceanRidgeOctaves", desc.ocean_ridge_octaves);
    gpu.SetFloat("oceanRidgeScale", desc.ocean_ridge_scale);
    gpu.SetFloat("oceanRidgeStrength", desc.ocean_ridge_strength);
    gpu.SetFloat("oceanRidgePower", desc.ocean_ridge_power);
    gpu.SetFloat("oceanRidgeGain", desc.ocean_ridge_gain);
    gpu.SetInt("trenchOctaves", desc.trench_octaves);
    gpu.SetFloat("trenchScale", desc.trench_scale);
    gpu.SetFloat("trenchDepth", desc.trench_depth);
    gpu.SetInt("abyssalOctaves", desc.abyssal_octaves);
    gpu.SetFloat("abyssalScale", desc.abyssal_scale);
    gpu.SetFloat("abyssalStrength", desc.abyssal_strength);
}

void SetShadingUniforms(planetgen::IComputeBackend& gpu, const PgShadingDesc& desc, uint32_t seed, uint32_t vertexCount)
{
    gpu.SetUint("numVertices", vertexCount);
    gpu.SetUint("seed", seed);
    gpu.SetFloat("detailNoiseScale", desc.detail_noise_scale);
    gpu.SetFloat("smallNoiseScale", desc.small_noise_scale);
    gpu.SetInt("smallNoiseOctaves", desc.small_noise_octaves);
    gpu.SetFloat("warpStrength", desc.warp_strength);

    gpu.SetInt("useClimateModel", desc.use_climate_model);
    gpu.SetFloat("largeNoiseScale", desc.large_noise_scale);
    gpu.SetInt("largeNoiseOctaves", desc.large_noise_octaves);
    gpu.SetFloat("temperatureLapseRate", desc.temperature_lapse_rate);
    gpu.SetFloat("temperatureExponent", desc.temperature_exponent);
    gpu.SetFloat("moistureNoiseScale", desc.moisture_noise_scale);
    gpu.SetFloat("moistureNoiseStrength", desc.moisture_noise_strength);
    gpu.SetFloat("hadleyIntensity", desc.hadley_intensity);
    gpu.SetFloat("continentalityStrength", desc.continentality_strength);
    gpu.SetFloat("heightScale", desc.height_scale);
}

void SetErosionUniforms(planetgen::IComputeBackend& gpu, const PgErosionDesc& desc, uint32_t vertexCount)
{
    gpu.SetUint("numVertices", vertexCount);
    gpu.SetInt("gridResolution", desc.grid_resolution);
    gpu.SetFloat("thermalRate", desc.thermal_rate);
    gpu.SetFloat("thermalThreshold", desc.thermal_threshold);
    gpu.SetFloat("hydraulicRate", desc.hydraulic_rate);
    gpu.SetFloat("depositionRate", desc.deposition_rate);
    gpu.SetFloat("evaporationRate", desc.evaporation_rate);
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

    auto backend = std::make_unique<planetgen::OpenGLBackend>();

    bool ok;
    if (desc->use_existing_context)
        ok = backend->InitWithExistingContext();
    else
        ok = backend->InitHeadless();

    if (!ok)
    {
        delete ctx;
        return nullptr;
    }

    ctx->backend = std::move(backend);

    // Compile embedded shaders
    ctx->heightShader = ctx->backend->CompileShader(planetgen::shaders::height_earth_comp);
    if (!ctx->heightShader)
    {
        ctx->lastError = PG_ERROR_SHADER_COMPILE_FAILED;
        std::cerr << "[planetgen] Failed to compile height shader" << std::endl;
    }

    ctx->shadingEarthShader = ctx->backend->CompileShader(planetgen::shaders::shading_earth_comp);
    if (!ctx->shadingEarthShader)
    {
        ctx->lastError = PG_ERROR_SHADER_COMPILE_FAILED;
        std::cerr << "[planetgen] Failed to compile earth shading shader" << std::endl;
    }

    ctx->shadingGenericShader = ctx->backend->CompileShader(planetgen::shaders::shading_generic_comp);
    if (!ctx->shadingGenericShader)
    {
        ctx->lastError = PG_ERROR_SHADER_COMPILE_FAILED;
        std::cerr << "[planetgen] Failed to compile generic shading shader" << std::endl;
    }

    ctx->erosionShader = ctx->backend->CompileShader(planetgen::shaders::erosion_earth_comp);
    if (!ctx->erosionShader)
    {
        ctx->lastError = PG_ERROR_SHADER_COMPILE_FAILED;
        std::cerr << "[planetgen] Failed to compile erosion shader" << std::endl;
    }

    return ctx;
}

void pg_context_destroy(PgContext ctx)
{
    if (!ctx)
        return;

    if (ctx->backend)
    {
        if (ctx->heightShader) ctx->backend->DestroyShader(ctx->heightShader);
        if (ctx->shadingEarthShader) ctx->backend->DestroyShader(ctx->shadingEarthShader);
        if (ctx->shadingGenericShader) ctx->backend->DestroyShader(ctx->shadingGenericShader);
        if (ctx->erosionShader) ctx->backend->DestroyShader(ctx->erosionShader);
    }

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

        // Parse terrain noise layers
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

    if (!ctx->heightShader)
    {
        ctx->lastError = PG_ERROR_NOT_INITIALIZED;
        return nullptr;
    }

    // Create GPU buffers
    size_t vertexBytes = vertex_count * 3 * sizeof(float);
    size_t heightBytes = vertex_count * sizeof(float);
    size_t normalBytes = vertex_count * 3 * sizeof(float);

    auto vertexBuf = gpu.CreateBuffer(vertexBytes);
    auto heightBuf = gpu.CreateBuffer(heightBytes);
    auto normalBuf = gpu.CreateBuffer(normalBytes);

    gpu.UploadBuffer(vertexBuf, vertices, vertexBytes);

    // Bind and dispatch
    gpu.BindShader(ctx->heightShader);
    gpu.BindBuffer(vertexBuf, 0);
    gpu.BindBuffer(heightBuf, 1);
    gpu.BindBuffer(normalBuf, 2);

    SetHeightUniforms(gpu, body->desc, seed, vertex_count);

    gpu.Dispatch(ComputeGroupCount(vertex_count, HeightWorkgroupSize));
    gpu.Barrier();

    // Readback
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

    // Choose shader based on climate model setting
    uint32_t shader = desc->use_climate_model ? ctx->shadingEarthShader : ctx->shadingGenericShader;
    if (!shader)
    {
        // Fall back to whichever is available
        shader = ctx->shadingEarthShader ? ctx->shadingEarthShader : ctx->shadingGenericShader;
        if (!shader)
        {
            ctx->lastError = PG_ERROR_NOT_INITIALIZED;
            return nullptr;
        }
    }

    size_t vertexBytes = vertex_count * 3 * sizeof(float);
    size_t heightBytes = vertex_count * sizeof(float);
    size_t shadingBytes = vertex_count * 4 * sizeof(float);

    auto vertexBuf = gpu.CreateBuffer(vertexBytes);
    auto shadingBuf = gpu.CreateBuffer(shadingBytes);
    auto heightBuf = gpu.CreateBuffer(heightBytes);

    gpu.UploadBuffer(vertexBuf, vertices, vertexBytes);
    gpu.UploadBuffer(heightBuf, heights, heightBytes);

    gpu.BindShader(shader);
    gpu.BindBuffer(vertexBuf, 0);
    gpu.BindBuffer(shadingBuf, 1);
    gpu.BindBuffer(heightBuf, 2);

    SetShadingUniforms(gpu, *desc, seed, vertex_count);

    gpu.Dispatch(ComputeGroupCount(vertex_count, ShadingWorkgroupSize));
    gpu.Barrier();

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

    if (!ctx->erosionShader)
    {
        ctx->lastError = PG_ERROR_NOT_INITIALIZED;
        return nullptr;
    }

    size_t heightBytes = vertex_count * sizeof(float);

    auto inputBuf = gpu.CreateBuffer(heightBytes);
    auto outputBuf = gpu.CreateBuffer(heightBytes);

    gpu.UploadBuffer(inputBuf, heights, heightBytes);

    gpu.BindShader(ctx->erosionShader);

    // Run iterations, ping-ponging between buffers
    for (int i = 0; i < desc->iterations; ++i)
    {
        gpu.BindBuffer(inputBuf, 0);
        gpu.BindBuffer(outputBuf, 1);

        SetErosionUniforms(gpu, *desc, vertex_count);

        gpu.Dispatch(ComputeGroupCount(vertex_count, ErosionWorkgroupSize));
        gpu.Barrier();

        // Swap for next iteration
        auto tmp = inputBuf;
        inputBuf = outputBuf;
        outputBuf = tmp;
    }

    // After all iterations, inputBuf holds the final result (due to last swap)
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
    // In a full implementation, result would ref-count or the consumer would manage GPU lifetime.
    // For now, just free the CPU-side data.
    delete result;
}

// ============================================================================
// Utility — default descriptors
// ============================================================================

void pg_body_desc_defaults(PgBodyDesc* desc)
{
    if (!desc)
        return;

    std::memset(desc, 0, sizeof(PgBodyDesc));

    // Continent noise
    desc->continent_noise.scale = 0.8f;
    desc->continent_noise.octaves = 6;
    desc->continent_noise.persistence = 0.5f;
    desc->continent_noise.lacunarity = 2.0f;
    desc->continent_noise.strength = 2.0f;

    // Mountain noise
    desc->mountain_noise.scale = 1.5f;
    desc->mountain_noise.octaves = 5;
    desc->mountain_noise.persistence = 0.5f;
    desc->mountain_noise.lacunarity = 4.0f;
    desc->mountain_noise.strength = 0.87f;

    // Mask noise
    desc->mask_noise.scale = 1.09f;
    desc->mask_noise.octaves = 3;
    desc->mask_noise.persistence = 0.55f;
    desc->mask_noise.lacunarity = 1.66f;
    desc->mask_noise.strength = 1.0f;

    desc->height_scale = 0.04f;
    desc->ocean_depth_multiplier = 5.0f;
    desc->ocean_floor_depth = 1.36f;
    desc->ocean_floor_smoothing = 0.5f;
    desc->mountain_blend = 1.16f;
    desc->continent_base_level = -0.35f;
    desc->global_frequency = 1.0f;
    desc->normal_epsilon = 0.0001f;

    desc->mountain_power = 2.18f;
    desc->mountain_gain = 0.8f;
    desc->mountain_smoothing = 1.0f;

    // Tectonics
    desc->use_tectonics = 1;
    desc->num_plates = 12;
    desc->continental_fraction = 0.45f;
    desc->boundary_width = 0.15f;
    desc->convergent_mountain_scale = 0.6f;
    desc->divergent_rift_depth = 0.3f;
    desc->coastline_noise = 0.4f;
    desc->plate_elevation_noise = 0.2f;

    // Ocean floor
    desc->use_ocean_floor = 1;
    desc->shelf_width = 0.15f;
    desc->ocean_ridge_octaves = 4;
    desc->ocean_ridge_scale = 0.8f;
    desc->ocean_ridge_strength = 0.3f;
    desc->ocean_ridge_power = 2.0f;
    desc->ocean_ridge_gain = 0.8f;
    desc->trench_octaves = 3;
    desc->trench_scale = 1.5f;
    desc->trench_depth = 0.4f;
    desc->abyssal_octaves = 4;
    desc->abyssal_scale = 2.0f;
    desc->abyssal_strength = 0.1f;

    // Detail
    desc->detail_low_threshold = -0.1f;
    desc->detail_high_threshold = 0.3f;
    desc->perturb_strength_low = 0.001f;
    desc->perturb_strength_high = 0.004f;
    desc->detail_octaves_low = 2;
    desc->detail_octaves_high = 5;
    desc->detail_persistence = 0.45f;
    desc->detail_lacunarity = 2.2f;
    desc->perturb_scale = 20.0f;
}

void pg_shading_desc_defaults(PgShadingDesc* desc)
{
    if (!desc)
        return;

    std::memset(desc, 0, sizeof(PgShadingDesc));

    desc->detail_noise_scale = 2.0f;
    desc->small_noise_scale = 15.0f;
    desc->small_noise_octaves = 5;
    desc->warp_strength = 0.3f;

    desc->use_climate_model = 1;
    desc->large_noise_scale = 0.3f;
    desc->large_noise_octaves = 3;
    desc->temperature_lapse_rate = 2.0f;
    desc->temperature_exponent = 0.6f;
    desc->moisture_noise_scale = 1.5f;
    desc->moisture_noise_strength = 0.15f;
    desc->hadley_intensity = 1.0f;
    desc->continentality_strength = 0.3f;
    desc->height_scale = 0.04f;
}

void pg_erosion_desc_defaults(PgErosionDesc* desc)
{
    if (!desc)
        return;

    std::memset(desc, 0, sizeof(PgErosionDesc));

    desc->iterations = 5;
    desc->grid_resolution = 256;
    desc->thermal_rate = 0.02f;
    desc->thermal_threshold = 0.01f;
    desc->hydraulic_rate = 0.01f;
    desc->deposition_rate = 0.005f;
    desc->evaporation_rate = 0.1f;
}
