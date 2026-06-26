#pragma once

// Canonical typed body configuration — the single source of truth for what a body is.
// PgBodyDesc/PgShadingDesc/PgErosionDesc are projections from this; they carry no
// independent defaults. Pure data: no GPU, JSON, or strategy knowledge.

#include <string>
#include <glm/glm.hpp>

namespace planetgen
{

// Per-layer noise descriptor used across multiple blocks
struct NoiseLayerConfig
{
    float scale       = 1.0f;
    int   octaves     = 4;
    float persistence = 0.5f;
    float lacunarity  = 2.0f;
    float strength    = 1.0f;
};

// ============================================================================
// Block 1 — Metadata
// ============================================================================

struct MetadataBlock
{
    std::string name             = "Unknown";
    std::string typeName         = "generic";
    float       radius           = 10.0f;
    float       seaLevel         = 0.995f;
    float       atmosphereHeight = 0.08f;
    bool        hasSolidSurface  = true;
    bool        hasAtmosphere    = true;
};

// ============================================================================
// Block 2 — Shape / Height
// ============================================================================

struct ShapeBlock
{
    NoiseLayerConfig continentNoise = { 0.8f, 6, 0.5f, 2.0f, 2.0f };
    NoiseLayerConfig mountainNoise  = { 1.5f, 5, 0.5f, 4.0f, 0.87f };
    NoiseLayerConfig maskNoise      = { 1.09f, 3, 0.55f, 1.66f, 1.0f };

    float heightScale          = 0.04f;
    float oceanDepthMultiplier = 5.0f;
    float oceanFloorDepth      = 1.36f;
    float oceanFloorSmoothing  = 0.5f;
    float mountainBlend        = 1.16f;
    float continentBaseLevel   = -0.35f;
    float globalFrequency      = 1.0f;
    float normalEpsilon        = 0.0001f;

    float mountainPower     = 2.18f;
    float mountainGain      = 0.8f;
    float mountainSmoothing = 1.0f;
};

// ============================================================================
// Block 3 — Tectonics
// ============================================================================

struct TectonicsBlock
{
    bool  enabled               = true;
    int   numPlates             = 12;
    float continentalFraction   = 0.45f;
    float boundaryWidth         = 0.15f;
    float convergentMountainScale = 0.6f;
    float divergentRiftDepth    = 0.3f;
    float coastlineNoise        = 0.4f;
    float plateElevationNoise   = 0.2f;
};

// ============================================================================
// Block 4 — Ocean floor topology
// ============================================================================

struct OceanFloorBlock
{
    bool  enabled           = true;
    float shelfWidth        = 0.15f;
    int   oceanRidgeOctaves = 4;
    float oceanRidgeScale   = 0.8f;
    float oceanRidgeStrength = 0.3f;
    float oceanRidgePower   = 2.0f;
    float oceanRidgeGain    = 0.8f;
    int   trenchOctaves     = 3;
    float trenchScale       = 1.5f;
    float trenchDepth       = 0.4f;
    int   abyssalOctaves    = 4;
    float abyssalScale      = 2.0f;
    float abyssalStrength   = 0.1f;
};

// ============================================================================
// Block 5 — Height-dependent detail
// ============================================================================

struct HeightDetailBlock
{
    float detailLowThreshold  = -0.1f;
    float detailHighThreshold = 0.3f;
    float perturbStrengthLow  = 0.001f;
    float perturbStrengthHigh = 0.004f;
    int   detailOctavesLow    = 2;
    int   detailOctavesHigh   = 5;
    float detailPersistence   = 0.45f;
    float detailLacunarity    = 2.2f;
    float perturbScale        = 20.0f;
};

// ============================================================================
// Block 6 — Shading / Climate
// Covers both compute-side noise and render-side biome/color params.
// ============================================================================

struct ShadingBlock
{
    // Compute-side noise
    float detailNoiseScale  = 2.0f;
    float smallNoiseScale   = 15.0f;
    int   smallNoiseOctaves = 5;
    float warpStrength      = 0.3f;

    // Climate model
    bool  useClimateModel       = true;
    float largeNoiseScale       = 0.3f;
    int   largeNoiseOctaves     = 3;
    float temperatureLapseRate  = 2.0f;
    float temperatureExponent   = 0.6f;
    float moistureNoiseScale    = 1.5f;
    float moistureNoiseStrength = 0.15f;
    float hadleyIntensity       = 1.0f;
    float continentalityStrength = 0.3f;

    // Fragment-shader biome controls
    bool  biomesEnabled       = true;
    float steepnessThreshold  = 0.3f;
    float flatToSteepBlend    = 0.15f;
    float snowLatitude        = 0.8f;
    float snowBlend           = 0.1f;
    float snowLine            = 0.85f;
    float shoreHeight         = 0.08f;
    float coastalDepthRange   = 0.04f;
    float aoStrength          = 0.35f;

    // Color blending (Earth-style fragment shader)
    float flatColBlend      = 1.5f;
    float flatColBlendNoise = 0.3f;

    // Earth color palette embedded in block (used only by earth body type)
    glm::vec3 colorShoreLow  = { 0.78f, 0.72f, 0.50f };
    glm::vec3 colorShoreHigh = { 0.62f, 0.55f, 0.40f };
    glm::vec3 colorFlatLowA  = { 0.45f, 0.55f, 0.30f };
    glm::vec3 colorFlatHighA = { 0.20f, 0.40f, 0.18f };
    glm::vec3 colorFlatLowB  = { 0.15f, 0.40f, 0.15f };
    glm::vec3 colorFlatHighB = { 0.10f, 0.28f, 0.12f };
    glm::vec3 colorSteepLow  = { 0.40f, 0.38f, 0.35f };
    glm::vec3 colorSteepHigh = { 0.30f, 0.28f, 0.26f };
    glm::vec3 colorSnow      = { 0.95f, 0.95f, 0.98f };
};

// ============================================================================
// Block 7 — Erosion
// ============================================================================

struct ErosionBlock
{
    bool  enabled         = false;
    int   iterations      = 5;
    int   gridResolution  = 256;
    float thermalRate     = 0.02f;
    float thermalThreshold = 0.01f;
    float hydraulicRate   = 0.01f;
    float depositionRate  = 0.005f;
    float evaporationRate = 0.1f;
};

// ============================================================================
// Block 8 — Palette reference
// ============================================================================

struct PaletteRefBlock
{
    std::string paletteId = "earth"; // Resolved via PaletteRegistry
};

// ============================================================================
// Shader path override block — body-type-specific paths
// Defaults target the Earth shaders; generic bodies override as needed.
// ============================================================================

struct ShaderPathsBlock
{
    std::string heightShaderPath   = "shaders/compute/height_earth.comp";
    std::string shadingShaderPath  = "shaders/compute/shading_earth.comp";
    std::string erosionShaderPath  = "shaders/compute/erosion_earth.comp";
    std::string vertexShaderPath   = "shaders/earth/planet_earth.vert";
    std::string fragmentShaderPath = "shaders/earth/planet_earth.frag";
};

// ============================================================================
// Root config — one object per body
// ============================================================================

struct BodyConfig
{
    MetadataBlock   metadata;
    ShapeBlock      shape;
    TectonicsBlock  tectonics;
    OceanFloorBlock oceanFloor;
    HeightDetailBlock heightDetail;
    ShadingBlock    shading;
    ErosionBlock    erosion;
    PaletteRefBlock paletteRef;
    ShaderPathsBlock shaderPaths;

    // Validate ranges and enable-consistency; returns empty string on success, reason on failure.
    std::string Validate() const;
};

} // namespace planetgen
