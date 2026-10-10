#include "MacroWorldPass.h"

#include <algorithm>
#include <cmath>

namespace planetgen
{

namespace
{
constexpr uint32_t kMacroGroupSize = 8; // local_size_x/y in every macro_*.comp
constexpr uint32_t kFaces = 6;
constexpr float kHalfPi = 1.57079632679f;

uint64_t Fnv1a(uint64_t h, const void* data, size_t bytes)
{
    const auto* p = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < bytes; ++i)
    {
        h ^= p[i];
        h *= 0x100000001b3ull;
    }
    return h;
}
} // namespace

uint32_t MacroResolution(const BodyConfig& config)
{
    return static_cast<uint32_t>(std::clamp(config.macroWorld.resolution, 32, 1024));
}

MacroParams MakeMacroParams(const BodyConfig& config, uint32_t seed)
{
    const auto& m = config.macroWorld;
    const auto& t = config.tectonics;
    const auto& s = config.shape;

    MacroParams p{};
    p.resolution = MacroResolution(config);
    p.seed = seed;
    p.numPlates = std::max(2, t.numPlates);
    p.boundaryWidth = t.boundaryWidth;

    p.convergentThreshold = m.convergentThreshold;
    p.convergentMountainScale = t.convergentMountainScale;
    p.divergentRiftDepth = t.divergentRiftDepth;
    p.plateElevationNoise = t.plateElevationNoise;

    p.continentScale = s.continentNoise.scale * s.globalFrequency;
    p.continentLayers = s.continentNoise.octaves;
    p.continentPersistence = s.continentNoise.persistence;
    p.continentLacunarity = s.continentNoise.lacunarity;

    // Same fixed continent offset the height stage uses (ParamMappers).
    p.continentOffsetX = 0.0f;
    p.continentOffsetY = 0.0f;
    p.continentOffsetZ = 0.0f;
    p.continentBaseLevel = s.continentBaseLevel;

    p.rainRate = m.rainRate;
    p.evaporationRate = m.evaporationRate;
    p.flowRate = m.flowRate;
    p.erosionRate = m.erosionRate;

    p.depositionRate = m.depositionRate;
    p.capacityMultiplier = m.capacityMultiplier;
    p.cellSizeMeters = kMacroPlanetRadiusMeters * kHalfPi / static_cast<float>(p.resolution);
    p.metersPerUnit = kMacroMetersPerUnit;

    p.windStrength = m.windStrength;
    p.landDrying = m.landDrying;
    p.orographicStrength = m.orographicStrength;
    p.temperatureLapseRate = config.shading.temperatureLapseRate;

    p.temperatureExponent = config.shading.temperatureExponent;
    // Upstream's resolution-aware channel threshold (orchestrator.gd, 18c1988).
    const float cellKm = p.cellSizeMeters / 1000.0f;
    p.channelFluxThreshold = std::clamp(0.008f * std::pow(std::max(cellKm, 0.01f), 0.25f), 0.005f, 0.04f);
    p.passIndex = 0;
    p.maxErosionPerPassMeters = 12.0f;
    return p;
}

MacroWorldBake::MacroWorldBake(IComputeBackend& backend, const ShaderRegistry& registry,
                               const BodyConfig& config, uint32_t seed)
    : _backend(backend), _registry(registry), _params(MakeMacroParams(config, seed)),
      _erosionIterations(config.macroWorld.erosionIterations), _res(_params.resolution)
{
    _ubo = _backend.CreateParamBuffer(sizeof(MacroParams));
}

MacroWorldBake::~MacroWorldBake()
{
    for (GpuTextureHandle t : { _geo, _geoTmp, _flux, _field })
        if (t)
            _backend.DestroyTexture(t);
    _backend.DestroyParamBuffer(_ubo);
}

GpuTextureHandle MacroWorldBake::NewArray()
{
    return _backend.CreateTexture2DArray_RGBA32F(_res, kFaces);
}

void MacroWorldBake::Dispatch(uint32_t program, uint32_t passIndex)
{
    _params.passIndex = passIndex;
    _backend.BindShader(program);
    _backend.SetParams(_ubo, &_params, sizeof(_params), kMacroParamBinding);
    const uint32_t groups = (_res + kMacroGroupSize - 1) / kMacroGroupSize;
    _backend.Dispatch(groups, groups, kFaces);
    _backend.ImageBarrier();
}

bool MacroWorldBake::Plates(GpuTextureHandle continentMask)
{
    const uint32_t program = _registry.Resolve(BuiltinProgram::MacroPlates);
    if (!program || !continentMask)
        return false;

    if (!_geo)
        _geo = NewArray();

    _backend.BindTexture3D(continentMask, kMacroMaskSamplerUnit);
    _backend.BindImage2DArray(_geo, 0);
    Dispatch(program, 0);
    return true;
}

bool MacroWorldBake::Erode(int iterations)
{
    const uint32_t rainfall = _registry.Resolve(BuiltinProgram::MacroErosionRainfall);
    const uint32_t flow = _registry.Resolve(BuiltinProgram::MacroErosionFlow);
    const uint32_t sediment = _registry.Resolve(BuiltinProgram::MacroErosionSediment);
    if (!rainfall || !flow || !sediment || !_geo)
        return false;

    if (!_geoTmp)
        _geoTmp = NewArray();
    if (!_flux)
        _flux = NewArray();

    // Each round ends back in _geo: rainfall in place, flow _geo -> _geoTmp,
    // sediment _geoTmp -> _geo. The flow pass zeroes the flux on round 0.
    for (int i = 0; i < iterations; ++i)
    {
        const uint32_t pass = static_cast<uint32_t>(i);

        _backend.BindImage2DArray(_geo, 0);
        Dispatch(rainfall, pass);

        _backend.BindImage2DArray(_geo, 0);
        _backend.BindImage2DArray(_geoTmp, 1);
        _backend.BindImage2DArray(_flux, 2);
        Dispatch(flow, pass);

        _backend.BindImage2DArray(_geoTmp, 0);
        _backend.BindImage2DArray(_geo, 1);
        _backend.BindImage2DArray(_flux, 2);
        Dispatch(sediment, pass);
    }
    return true;
}

bool MacroWorldBake::RunAll(GpuTextureHandle continentMask)
{
    if (!Plates(continentMask) || !Erode(_erosionIterations))
        return false;

    // Until the later stages exist the geo texture is the field.
    _field = _geo;
    _geo = 0;
    return true;
}

GpuTextureHandle MacroWorldBake::TakeField()
{
    const GpuTextureHandle field = _field;
    _field = 0;
    return field;
}

GpuTextureHandle BakeMacroWorld(IComputeBackend& backend, const ShaderRegistry& registry,
                                const BodyConfig& config, uint32_t seed,
                                GpuTextureHandle continentMask)
{
    MacroWorldBake bake(backend, registry, config, seed);
    if (!bake.RunAll(continentMask))
        return 0;
    return bake.TakeField();
}

uint64_t MacroCacheKey(const BodyConfig& config, uint32_t seed)
{
    const MacroParams p = MakeMacroParams(config, seed);
    const auto& s = config.shape;

    uint64_t h = 0xcbf29ce484222325ull;
    h = Fnv1a(h, &p, sizeof(p));
    h = Fnv1a(h, &config.macroWorld.erosionIterations, sizeof(int));
    h = Fnv1a(h, &config.macroWorld.moistureIterations, sizeof(int));
    h = Fnv1a(h, &s.continentCount, sizeof(int));
    h = Fnv1a(h, &s.continentSizeVariance, sizeof(float));
    h = Fnv1a(h, &s.continentClustering, sizeof(float));
    h = Fnv1a(h, &s.continentMaskResolution, sizeof(int));
    return h;
}

GpuTextureHandle EnsureMacroWorld(IComputeBackend& backend, const ShaderRegistry& registry,
                                  const BodyConfig& config, uint32_t seed,
                                  GpuTextureHandle continentMask,
                                  GpuTextureHandle& cachedTexture, uint64_t& cachedKey)
{
    // The bake needs the continent mask as its land/ocean authority; no mask, no layer.
    if (!config.macroWorld.enabled || !continentMask)
        return 0;

    const uint64_t key = MacroCacheKey(config, seed);
    if (cachedTexture != 0 && cachedKey == key)
        return cachedTexture;

    if (cachedTexture != 0)
        backend.DestroyTexture(cachedTexture);

    cachedTexture = BakeMacroWorld(backend, registry, config, seed, continentMask);
    cachedKey = (cachedTexture != 0) ? key : 0;
    return cachedTexture;
}

} // namespace planetgen
