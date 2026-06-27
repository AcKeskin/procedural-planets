// Public C API for libplanetgen — GPU-accelerated procedural terrain generation
// Designed for maximum compatibility: C# P/Invoke, Unreal FFI, any C consumer

#ifndef PLANETGEN_H
#define PLANETGEN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Platform export/import macros
#ifdef _WIN32
    #ifdef PLANETGEN_EXPORTS
        #define PG_API __declspec(dllexport)
    #else
        #define PG_API __declspec(dllimport)
    #endif
#else
    #define PG_API __attribute__((visibility("default")))
#endif

// Opaque handle types
typedef struct PgContext_T* PgContext;
typedef struct PgBody_T* PgBody;
typedef struct PgResult_T* PgResult;

// Error codes
typedef enum PgError
{
    PG_SUCCESS = 0,
    PG_ERROR_INVALID_ARGUMENT = -1,
    PG_ERROR_GPU_INIT_FAILED = -2,
    PG_ERROR_SHADER_COMPILE_FAILED = -3,
    PG_ERROR_OUT_OF_MEMORY = -4,
    PG_ERROR_FILE_NOT_FOUND = -5,
    PG_ERROR_JSON_PARSE_FAILED = -6,
    PG_ERROR_NOT_INITIALIZED = -7,
} PgError;

// Context configuration
typedef struct PgContextDesc
{
    int use_existing_context; // 1 = assume valid GL context on calling thread, 0 = create headless
} PgContextDesc;

// Noise layer configuration
typedef struct PgNoiseLayer
{
    float scale;
    int octaves;
    float persistence;
    float lacunarity;
    float strength;
} PgNoiseLayer;

// Body configuration for procedural generation
typedef struct PgBodyDesc
{
    // Terrain noise
    PgNoiseLayer continent_noise;
    PgNoiseLayer mountain_noise;
    PgNoiseLayer mask_noise;

    float height_scale;
    float ocean_depth_multiplier;
    float ocean_floor_depth;
    float ocean_floor_smoothing;
    float mountain_blend;
    float continent_base_level;
    float global_frequency;
    float normal_epsilon;

    // Mountain ridge parameters
    float mountain_power;
    float mountain_gain;
    float mountain_smoothing;

    // Tectonic plates (0 to disable)
    int use_tectonics;
    int num_plates;
    float continental_fraction;
    float boundary_width;
    float convergent_mountain_scale;
    float divergent_rift_depth;
    float coastline_noise;
    float plate_elevation_noise;

    // Ocean floor topology (0 to disable)
    int use_ocean_floor;
    float shelf_width;
    int ocean_ridge_octaves;
    float ocean_ridge_scale;
    float ocean_ridge_strength;
    float ocean_ridge_power;
    float ocean_ridge_gain;
    int trench_octaves;
    float trench_scale;
    float trench_depth;
    int abyssal_octaves;
    float abyssal_scale;
    float abyssal_strength;

    // Height-dependent detail
    float detail_low_threshold;
    float detail_high_threshold;
    float perturb_strength_low;
    float perturb_strength_high;
    int detail_octaves_low;
    int detail_octaves_high;
    float detail_persistence;
    float detail_lacunarity;
    float perturb_scale;
} PgBodyDesc;

// Shading configuration
typedef struct PgShadingDesc
{
    float detail_noise_scale;
    float small_noise_scale;
    int small_noise_octaves;
    float warp_strength;

    // Climate model (set use_climate_model = 0 for legacy mode)
    int use_climate_model;
    float large_noise_scale;
    int large_noise_octaves;
    float temperature_lapse_rate;
    float temperature_exponent;
    float moisture_noise_scale;
    float moisture_noise_strength;
    float hadley_intensity;
    float continentality_strength;
    float height_scale; // For normalization
} PgShadingDesc;

// One palette color stop. color = RGB in [0,1]; parameter is the body-specific
// threshold/weight the renderer interprets (matches the internal PaletteEntry).
typedef struct PgPaletteEntry
{
    float color[3];
    float parameter;
} PgPaletteEntry;

// Erosion configuration
typedef struct PgErosionDesc
{
    int iterations;
    int grid_resolution;
    float thermal_rate;
    float thermal_threshold;
    float hydraulic_rate;
    float deposition_rate;
    float evaporation_rate;
} PgErosionDesc;

// ============================================================================
// Context lifecycle
// ============================================================================

PG_API PgContext pg_context_create(const PgContextDesc* desc);
PG_API void     pg_context_destroy(PgContext ctx);
PG_API PgError  pg_context_get_error(PgContext ctx);

// Human-readable message for the last error on this context. Returns a pointer
// valid until the next call on the same context (do not free, do not retain).
// Returns "" when there is no error and a static string when ctx is null.
PG_API const char* pg_get_last_error_message(PgContext ctx);

// ============================================================================
// Body configuration
// ============================================================================

PG_API PgBody pg_body_create(PgContext ctx, const PgBodyDesc* desc);
PG_API PgBody pg_body_create_from_json(PgContext ctx, const char* json_path);
PG_API void   pg_body_destroy(PgBody body);

// ============================================================================
// Generation dispatch
//
// Determinism guarantee: for a fixed backend, identical (body config, seed,
// geometry) yields byte-identical per-vertex output across calls and processes.
// Generation is stateless per call — no hidden session state affects the result.
//
// Error model: every call below returns a handle (null on failure). On failure
// the owning context records a status code (pg_context_get_error) and a
// human-readable message (pg_get_last_error_message). No exception crosses the
// C ABI — internal exceptions are translated to code + message.
// ============================================================================

// Generate terrain heights and normals. vertices = packed float3 array (x,y,z per vertex).
// Returns result containing float heights and float3 normals.
PG_API PgResult pg_generate_heights(PgBody body, const float* vertices, uint32_t vertex_count, uint32_t seed);

// Generate per-vertex shading data (vec4 per vertex).
// heights = output from pg_generate_heights (needed for elevation-based shading).
PG_API PgResult pg_generate_shading(PgBody body, const float* vertices, const float* heights,
                                     uint32_t vertex_count, uint32_t seed, const PgShadingDesc* desc);

// Run erosion simulation on height data (modifies heights in-place conceptually).
PG_API PgResult pg_generate_erosion(PgBody body, const float* heights, uint32_t vertex_count,
                                     const PgErosionDesc* desc, uint32_t seed);

// Optional convenience for callers without their own geometry: build a UV-sphere
// mesh (rings x segments) and generate full per-vertex data + palette for it in
// one call. The result carries vertices (packed float3) and triangle indices in
// addition to heights/normals/shading/palette.
PG_API PgResult pg_generate_mesh(PgBody body, uint32_t rings, uint32_t segments, uint32_t seed);

// ============================================================================
// Result access
// ============================================================================

PG_API const float* pg_result_get_heights(PgResult result);
PG_API const float* pg_result_get_normals(PgResult result);   // Packed float3 (x,y,z per vertex)
PG_API const float* pg_result_get_shading(PgResult result);   // Packed float4 (x,y,z,w per vertex)
PG_API uint32_t     pg_result_get_count(PgResult result);
PG_API uint32_t     pg_result_get_gpu_buffer(PgResult result); // OpenGL SSBO handle (0 if CPU-only)

// Resolved palette for the body (from its palette id). Entries are owned by the
// result and valid until pg_result_destroy. Count is 0 when no palette resolved.
PG_API const PgPaletteEntry* pg_result_get_palette(PgResult result);
PG_API uint32_t              pg_result_get_palette_count(PgResult result);

// Mesh data — non-null only for results from pg_generate_mesh.
PG_API const float*    pg_result_get_vertices(PgResult result);    // packed float3
PG_API const uint32_t* pg_result_get_indices(PgResult result);     // triangle list
PG_API uint32_t        pg_result_get_index_count(PgResult result);

PG_API void         pg_result_destroy(PgResult result);

// ============================================================================
// Utility
// ============================================================================

// Fill a PgBodyDesc with sensible Earth-like defaults
PG_API void pg_body_desc_defaults(PgBodyDesc* desc);

// Fill a PgShadingDesc with sensible Earth-like defaults
PG_API void pg_shading_desc_defaults(PgShadingDesc* desc);

// Fill a PgErosionDesc with sensible defaults
PG_API void pg_erosion_desc_defaults(PgErosionDesc* desc);

#ifdef __cplusplus
}
#endif

#endif // PLANETGEN_H