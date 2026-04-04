#include "GenericBody.h"
#include "ComputeShader.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <glm/glm.hpp>

namespace planets::render
{

namespace
{
const glm::vec3 ContinentNoiseOffset(0.0f);
const glm::vec3 MountainNoiseOffset(1000.0f);
const glm::vec3 MaskNoiseOffset(2000.0f);
constexpr float DefaultMaskMultiplier = 1.0f;

GenericNoiseLayer ParseNoiseLayer(const nlohmann::json& json, const GenericNoiseLayer& defaults)
{
    GenericNoiseLayer layer = defaults;
    if (json.contains("scale")) layer.scale = json["scale"].get<float>();
    if (json.contains("octaves")) layer.octaves = json["octaves"].get<int>();
    if (json.contains("persistence")) layer.persistence = json["persistence"].get<float>();
    if (json.contains("lacunarity")) layer.lacunarity = json["lacunarity"].get<float>();
    if (json.contains("strength")) layer.strength = json["strength"].get<float>();
    return layer;
}
} // namespace

std::unique_ptr<GenericBody> GenericBody::LoadFromJson(const std::string& configPath)
{
    std::ifstream file(configPath);
    if (!file.is_open())
    {
        std::cerr << "[GenericBody] Failed to open config: " << configPath << std::endl;
        return nullptr;
    }

    try
    {
        nlohmann::json json;
        file >> json;

        auto body = std::make_unique<GenericBody>();
        auto& cfg = body->_config;

        // Core properties
        cfg.name = json.value("name", cfg.name);
        cfg.radius = json.value("radius", cfg.radius);
        cfg.seaLevel = json.value("seaLevel", cfg.seaLevel);
        cfg.atmosphereHeight = json.value("atmosphereHeight", cfg.atmosphereHeight);
        cfg.hasSolidSurface = json.value("hasSolidSurface", cfg.hasSolidSurface);
        cfg.hasAtmosphere = json.value("hasAtmosphere", cfg.hasAtmosphere);

        // Apply physical properties to base class
        body->SetRadius(cfg.radius);
        body->SetSeaLevel(cfg.seaLevel);
        body->SetAtmosphereHeight(cfg.atmosphereHeight);

        // Shader paths
        if (json.contains("shaders"))
        {
            auto& s = json["shaders"];
            cfg.heightShaderPath = s.value("height", cfg.heightShaderPath);
            cfg.shadingShaderPath = s.value("shading", cfg.shadingShaderPath);
            cfg.vertexShaderPath = s.value("vertex", cfg.vertexShaderPath);
            cfg.fragmentShaderPath = s.value("fragment", cfg.fragmentShaderPath);
        }

        // Palette path
        cfg.palettePath = json.value("palette", cfg.palettePath);

        // Terrain noise
        if (json.contains("terrain"))
        {
            auto& t = json["terrain"];
            cfg.heightScale = t.value("heightScale", cfg.heightScale);
            cfg.oceanDepthMultiplier = t.value("oceanDepthMultiplier", cfg.oceanDepthMultiplier);
            cfg.oceanFloorDepth = t.value("oceanFloorDepth", cfg.oceanFloorDepth);
            cfg.mountainBlend = t.value("mountainBlend", cfg.mountainBlend);

            if (t.contains("continent")) cfg.continentNoise = ParseNoiseLayer(t["continent"], cfg.continentNoise);
            if (t.contains("mountain")) cfg.mountainNoise = ParseNoiseLayer(t["mountain"], cfg.mountainNoise);
            if (t.contains("mask")) cfg.maskNoise = ParseNoiseLayer(t["mask"], cfg.maskNoise);
        }

        // Shading noise
        if (json.contains("shading"))
        {
            auto& s = json["shading"];
            cfg.detailNoiseScale = s.value("detailNoiseScale", cfg.detailNoiseScale);
            cfg.smallNoiseScale = s.value("smallNoiseScale", cfg.smallNoiseScale);
            cfg.detailNoiseOctaves = s.value("detailNoiseOctaves", cfg.detailNoiseOctaves);
            cfg.smallNoiseOctaves = s.value("smallNoiseOctaves", cfg.smallNoiseOctaves);
            cfg.warpStrength = s.value("warpStrength", cfg.warpStrength);
        }

        std::cout << "[GenericBody] Loaded '" << cfg.name << "' from " << configPath << std::endl;
        return body;
    }
    catch (const nlohmann::json::exception& e)
    {
        std::cerr << "[GenericBody] JSON parse error in " << configPath << ": " << e.what() << std::endl;
        return nullptr;
    }
}

std::string GenericBody::GetHeightShaderPath() const { return _config.heightShaderPath; }
std::string GenericBody::GetShadingShaderPath() const { return _config.shadingShaderPath; }
std::string GenericBody::GetVertexShaderPath() const { return _config.vertexShaderPath; }
std::string GenericBody::GetFragmentShaderPath() const { return _config.fragmentShaderPath; }

BiomePalette GenericBody::LoadBiomePalette() const
{
    if (_config.palettePath.empty())
        return {};
    return BiomePalette::LoadFromJson(_config.palettePath);
}

void GenericBody::SetShapeUniforms(ComputeShader& shader, uint32_t seed) const
{
    shader.Use();
    shader.SetUint("seed", seed);

    // Continental noise
    shader.SetVec3("continentOffset", ContinentNoiseOffset);
    shader.SetInt("continentLayers", _config.continentNoise.octaves);
    shader.SetFloat("continentScale", _config.continentNoise.scale);
    shader.SetFloat("continentPersistence", _config.continentNoise.persistence);
    shader.SetFloat("continentLacunarity", _config.continentNoise.lacunarity);
    shader.SetFloat("continentMultiplier", _config.continentNoise.strength);

    // Mountain noise
    shader.SetVec3("mountainOffset", MountainNoiseOffset);
    shader.SetInt("mountainLayers", _config.mountainNoise.octaves);
    shader.SetFloat("mountainScale", _config.mountainNoise.scale);
    shader.SetFloat("mountainPersistence", _config.mountainNoise.persistence);
    shader.SetFloat("mountainLacunarity", _config.mountainNoise.lacunarity);
    shader.SetFloat("mountainMultiplier", _config.mountainNoise.strength);
    shader.SetFloat("mountainPower", 1.0f);
    shader.SetFloat("mountainGain", 1.0f);
    shader.SetFloat("mountainSmooth", 1.0f);

    // Mask noise
    shader.SetVec3("maskOffset", MaskNoiseOffset);
    shader.SetInt("maskLayers", _config.maskNoise.octaves);
    shader.SetFloat("maskScale", _config.maskNoise.scale);
    shader.SetFloat("maskPersistence", _config.maskNoise.persistence);
    shader.SetFloat("maskLacunarity", _config.maskNoise.lacunarity);
    shader.SetFloat("maskMultiplier", DefaultMaskMultiplier);

    // Terrain blending
    shader.SetFloat("oceanDepthMultiplier", _config.oceanDepthMultiplier);
    shader.SetFloat("oceanFloorDepth", _config.oceanFloorDepth);
    shader.SetFloat("oceanFloorSmoothing", 0.5f);
    shader.SetFloat("mountainBlend", _config.mountainBlend);
    shader.SetFloat("heightScale", _config.heightScale);
    shader.SetFloat("continentBaseLevel", 0.0f);

    // Disable Earth-specific features
    shader.SetInt("useTectonics", 0);
    shader.SetInt("useOceanFloor", 0);

    // Detail noise defaults
    shader.SetFloat("detailLowThreshold", 0.0f);
    shader.SetFloat("detailHighThreshold", 1.0f);
    shader.SetFloat("perturbStrengthLow", 0.0f);
    shader.SetFloat("perturbStrengthHigh", 0.0f);
    shader.SetInt("detailOctavesLow", 0);
    shader.SetInt("detailOctavesHigh", 0);
    shader.SetFloat("detailPersistence", 0.5f);
    shader.SetFloat("detailLacunarity", 2.0f);
    shader.SetFloat("perturbScale", 1.0f);
    shader.SetFloat("globalFrequency", 1.0f);
    shader.SetFloat("normalEpsilon", 0.0001f);
}

void GenericBody::SetShadingUniforms(ComputeShader& shader, uint32_t seed) const
{
    shader.Use();
    shader.SetUint("seed", seed);
    shader.SetFloat("heightScale", _config.heightScale);
    shader.SetFloat("detailNoiseScale", _config.detailNoiseScale);
    shader.SetFloat("smallNoiseScale", _config.smallNoiseScale);
    shader.SetInt("detailNoiseOctaves", _config.detailNoiseOctaves);
    shader.SetInt("smallNoiseOctaves", _config.smallNoiseOctaves);
    shader.SetFloat("warpStrength", _config.warpStrength);
}

} // namespace planets::render
