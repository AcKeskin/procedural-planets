#pragma once

#include "planetgen/planetgen.h"
#include "../backend/IComputeBackend.h"
#include "../backend/ShaderRegistry.h"
#include "../model/PaletteRegistry.h"
#include "../model/BodyConfig.h"
#include <memory>
#include <string>
#include <vector>

// Result types (internal)
enum class PgResultType
{
    Heights,
    Shading,
    Erosion
};

// Internal context — owns the compute backend and shader registry.
// Named shader fields removed; the registry owns all program handles now.
// Defined at global scope to match C API forward declaration.
struct PgContext_T
{
    std::unique_ptr<planetgen::IComputeBackend> backend;
    planetgen::ShaderRegistry registry;
    planetgen::PaletteRegistry palettes; // built-in palettes registered in ctor

    // Last-error code + human-readable message. Message is valid until the next
    // call on this context. SetError sets both; ClearError marks success.
    PgError     lastError = PG_SUCCESS;
    std::string lastErrorMessage;

    void SetError(PgError code, std::string message)
    {
        lastError = code;
        lastErrorMessage = std::move(message);
    }

    void ClearError()
    {
        lastError = PG_SUCCESS;
        lastErrorMessage.clear();
    }
};

// Internal body — stores generation parameters.
// `config` is the composable typed model (the canonical shape, loaded from JSON);
// `desc` is its flat projection the strategies bind. For bodies created from the
// flat PgBodyDesc directly, `config` stays default and `desc` is authoritative.
//
// The continent-mask texture is cached per body, keyed on (seed, resolution). It is
// a deterministic function of those inputs, so caching it does not make generation
// stateful — re-baking would reproduce the same volume.
struct PgBody_T
{
    PgContext_T* ctx = nullptr;
    PgBodyDesc desc{};
    planetgen::BodyConfig config{};

    planetgen::GpuTextureHandle maskTexture = 0; // 0 = not yet baked
    uint64_t maskBuiltKey = 0;

    ~PgBody_T()
    {
        if (maskTexture && ctx && ctx->backend)
            ctx->backend->DestroyTexture(maskTexture);
    }
};

// Internal result — owns CPU readback data
struct PgResult_T
{
    PgResultType type = PgResultType::Heights;
    uint32_t count = 0;
    std::vector<float> heights;
    std::vector<float> normals;  // Packed float3 for height results
    std::vector<float> shading;  // Packed float4 for shading results
    std::vector<PgPaletteEntry> palette; // resolved body palette (may be empty)

    // Mesh data — populated only by pg_generate_mesh.
    std::vector<float>    vertices; // packed float3
    std::vector<uint32_t> indices;  // triangle list

    // GPU buffer handles (valid while context is alive, 0 if readback-only)
    uint32_t heightsGpuBuffer = 0;
    uint32_t normalsGpuBuffer = 0;
    uint32_t shadingGpuBuffer = 0;
};
