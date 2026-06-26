#pragma once

#include "planetgen/planetgen.h"
#include "../backend/IComputeBackend.h"
#include "../backend/ShaderRegistry.h"
#include <memory>
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
    PgError lastError = PG_SUCCESS;
};

// Internal body — stores generation parameters
struct PgBody_T
{
    PgContext_T* ctx = nullptr;
    PgBodyDesc desc{};
};

// Internal result — owns CPU readback data
struct PgResult_T
{
    PgResultType type = PgResultType::Heights;
    uint32_t count = 0;
    std::vector<float> heights;
    std::vector<float> normals;  // Packed float3 for height results
    std::vector<float> shading;  // Packed float4 for shading results

    // GPU buffer handles (valid while context is alive, 0 if readback-only)
    uint32_t heightsGpuBuffer = 0;
    uint32_t normalsGpuBuffer = 0;
    uint32_t shadingGpuBuffer = 0;
};
