#pragma once

// std140-compatible parameter structs for all four compute shaders.
//
// Layout rules enforced here:
//   - vec3  → 16-byte slot (float[4] with last component unused)
//   - scalars → packed 4 per 16-byte row where order allows
//   - Every struct is alignas(16) per std140 aggregate rule
//   - static_asserts pin the sizeof so a bad compiler default won't silently misalign
//
// Each struct has a matching layout(std140) block in its .comp file.
// Field ORDER must match the GLSL declaration order exactly.

#include <cstddef>
#include <cstdint>

namespace planetgen
{

// ============================================================================
// HeightParams — binding = 3 in height_earth.comp
// ============================================================================
struct alignas(16) HeightParams
{
    // Row 0
    uint32_t numVertices;
    uint32_t seed;
    int32_t  useTectonics;
    int32_t  numPlates;

    // Row 1
    float continentalFraction;
    float boundaryWidth;
    float convergentMountainScale;
    float divergentRiftDepth;

    // Row 2
    float coastlineNoise;
    float plateElevationNoise;
    float oceanDepthMultiplier;
    float oceanFloorDepth;

    // Row 3
    float oceanFloorSmoothing;
    float mountainBlend;
    float heightScale;
    float continentBaseLevel;

    // Row 4
    float globalFrequency;
    float normalEpsilon;
    float mountainPower;
    float mountainGain;

    // Row 5
    float mountainSmooth;
    int32_t  continentLayers;
    float continentScale;
    float continentPersistence;

    // Row 6
    float continentLacunarity;
    float continentMultiplier;
    int32_t  mountainLayers;
    float mountainScale;

    // Row 7
    float mountainPersistence;
    float mountainLacunarity;
    float mountainMultiplier;
    int32_t  maskLayers;

    // Row 8
    float maskScale;
    float maskPersistence;
    float maskLacunarity;
    float maskMultiplier;

    // Row 9 — continent offset (vec3 → 16-byte slot)
    float continentOffsetX;
    float continentOffsetY;
    float continentOffsetZ;
    float _pad0;

    // Row 10 — mountain offset
    float mountainOffsetX;
    float mountainOffsetY;
    float mountainOffsetZ;
    float _pad1;

    // Row 11 — mask offset
    float maskOffsetX;
    float maskOffsetY;
    float maskOffsetZ;
    float _pad2;

    // Row 12
    float detailLowThreshold;
    float detailHighThreshold;
    float perturbStrengthLow;
    float perturbStrengthHigh;

    // Row 13
    int32_t detailOctavesLow;
    int32_t detailOctavesHigh;
    float detailPersistence;
    float detailLacunarity;

    // Row 14
    float perturbScale;
    int32_t useOceanFloor;
    float shelfWidth;
    int32_t oceanRidgeOctaves;

    // Row 15
    float oceanRidgeScale;
    float oceanRidgeStrength;
    float oceanRidgePower;
    float oceanRidgeGain;

    // Row 16
    int32_t trenchOctaves;
    float trenchScale;
    float trenchDepth;
    int32_t abyssalOctaves;

    // Row 17
    float abyssalScale;
    float abyssalStrength;
    float _pad3;
    float _pad4;
};

static_assert(sizeof(HeightParams) == 18 * 16, "HeightParams std140 layout mismatch");
static_assert(offsetof(HeightParams, continentOffsetX) == 9 * 16,
              "HeightParams continentOffset must start at row 9");
static_assert(offsetof(HeightParams, mountainOffsetX) == 10 * 16,
              "HeightParams mountainOffset must start at row 10");
static_assert(offsetof(HeightParams, maskOffsetX) == 11 * 16,
              "HeightParams maskOffset must start at row 11");

// ============================================================================
// ShadingEarthParams — binding = 3 in shading_earth.comp
// ============================================================================
struct alignas(16) ShadingEarthParams
{
    // Row 0
    uint32_t numVertices;
    uint32_t seed;
    int32_t  useClimateModel;
    int32_t  largeNoiseOctaves;

    // Row 1
    float largeNoiseScale;
    float detailNoiseScale;
    float smallNoiseScale;
    int32_t  smallNoiseOctaves;

    // Row 2
    float warpStrength;
    float temperatureLapseRate;
    float temperatureExponent;
    float moistureNoiseScale;

    // Row 3
    float moistureNoiseStrength;
    float hadleyIntensity;
    float continentalityStrength;
    float heightScale;
};

static_assert(sizeof(ShadingEarthParams) == 4 * 16, "ShadingEarthParams std140 layout mismatch");

// ============================================================================
// ShadingGenericParams — binding = 3 in shading_generic.comp
// ============================================================================
struct alignas(16) ShadingGenericParams
{
    // Row 0
    uint32_t numVertices;
    uint32_t seed;
    float    heightScale;
    float    detailNoiseScale;

    // Row 1
    float smallNoiseScale;
    int32_t  smallNoiseOctaves;
    float warpStrength;
    float _pad0;
};

static_assert(sizeof(ShadingGenericParams) == 2 * 16, "ShadingGenericParams std140 layout mismatch");

// ============================================================================
// ErosionParams — binding = 3 in erosion_earth.comp
// ============================================================================
struct alignas(16) ErosionParams
{
    // Row 0
    uint32_t numVertices;
    int32_t  gridResolution;
    float thermalRate;
    float thermalThreshold;

    // Row 1
    float hydraulicRate;
    float depositionRate;
    float evaporationRate;
    float _pad0;
};

static_assert(sizeof(ErosionParams) == 2 * 16, "ErosionParams std140 layout mismatch");

// ============================================================================
// ContinentGrowthParams — binding = 1 in continent_growth.comp
// ============================================================================
struct alignas(16) ContinentGrowthParams
{
    // Row 0
    uint32_t count;
    uint32_t seed;
    uint32_t resolution;
    uint32_t iterations;

    // Row 1
    float sizeVariance;
    float clustering;
    float _pad0;
    float _pad1;
};

static_assert(sizeof(ContinentGrowthParams) == 2 * 16, "ContinentGrowthParams std140 layout mismatch");

} // namespace planetgen
