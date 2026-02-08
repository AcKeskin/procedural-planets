#include "Earth.h"
#include "ComputeShader.h"
#include <glm/glm.hpp>

namespace planets::render
{

Earth::Earth()
{
    // Default Earth-like settings
    _radius = 10.0f;
    _seaLevel = 0.4f;
    _atmosphereHeight = 0.08f;
}

void Earth::SetShapeUniforms(ComputeShader& shader, uint32_t seed) const
{
    shader.Use();
    shader.SetUint("seed", seed);

    // Continental noise configuration
    shader.SetVec3("continentOffset", glm::vec3(0.0f));
    shader.SetInt("continentLayers", _terrainSettings.continentOctaves);
    shader.SetFloat("continentScale", _terrainSettings.continentScale);
    shader.SetFloat("continentPersistence", _terrainSettings.continentPersistence);
    shader.SetFloat("continentLacunarity", _terrainSettings.continentLacunarity);
    shader.SetFloat("continentMultiplier", _terrainSettings.continentStrength);

    // Mountain ridge noise configuration
    shader.SetVec3("mountainOffset", glm::vec3(1000.0f));
    shader.SetInt("mountainLayers", _terrainSettings.mountainOctaves);
    shader.SetFloat("mountainScale", _terrainSettings.mountainScale);
    shader.SetFloat("mountainPersistence", _terrainSettings.mountainPersistence);
    shader.SetFloat("mountainLacunarity", _terrainSettings.mountainLacunarity);
    shader.SetFloat("mountainMultiplier", _terrainSettings.mountainStrength);
    shader.SetFloat("mountainPower", _terrainSettings.mountainPower);
    shader.SetFloat("mountainGain", _terrainSettings.mountainGain);
    shader.SetFloat("mountainSmooth", _terrainSettings.mountainSmoothing);

    // Mask noise configuration
    shader.SetVec3("maskOffset", glm::vec3(2000.0f));
    shader.SetInt("maskLayers", _terrainSettings.maskOctaves);
    shader.SetFloat("maskScale", _terrainSettings.maskScale);
    shader.SetFloat("maskPersistence", _terrainSettings.maskPersistence);
    shader.SetFloat("maskLacunarity", _terrainSettings.maskLacunarity);
    shader.SetFloat("maskMultiplier", 1.0f);

    // Ocean and terrain blending
    shader.SetFloat("oceanDepthMultiplier", _terrainSettings.oceanDepthMultiplier);
    shader.SetFloat("oceanFloorDepth", _terrainSettings.oceanFloorDepth);
    shader.SetFloat("oceanFloorSmoothing", _terrainSettings.oceanFloorSmoothing);
    shader.SetFloat("mountainBlend", _terrainSettings.mountainBlend);
    shader.SetFloat("heightScale", _terrainSettings.heightScale);
}

void Earth::SetShadingUniforms(ComputeShader& shader, uint32_t seed) const
{
    // Future: biome noise parameters (temperature, moisture, vegetation)
    // For now, placeholder for extensibility
    shader.Use();
    shader.SetUint("shadingSeed", seed);
}

} // namespace planets::render
