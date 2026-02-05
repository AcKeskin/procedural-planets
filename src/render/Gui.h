#pragma once

#include <cstdint>
#include <glm/glm.hpp>

struct GLFWwindow;

namespace planets::core {
class Camera;
class Planet;
}

namespace planets::render {

struct EarthTerrainSettings;
struct EarthShadingSettings;
struct EarthColors;

namespace effects {
struct OceanSettings;
struct AtmosphereSettings;
}

// Biome settings for shader - height zones from sea level
// All height thresholds are RELATIVE (0 = sea level, 1 = max terrain height)
struct BiomeSettings
{
    bool enabled = true;

    // Steepness controls (for rock on cliffs)
    float steepnessThreshold = 0.3f;   // Steepness value where rock starts (0-1)
    float flatToSteepBlend = 0.15f;    // Blend width for flat-to-rock transition

    // Snow controls
    float snowLatitude = 0.8f;         // Latitude (0=equator, 1=pole) for polar snow
    float snowBlend = 0.1f;            // Blend width for latitude snow
    float snowLine = 0.85f;            // Height (0-1) where snow begins on peaks

    // Shore/beach zone
    float shoreHeight = 0.08f;         // Height (0-1) of shore zone above sea level

    // Height range override for normalization
    bool autoHeightRange = true;
    float heightRangeMin = 0.96f;
    float heightRangeMax = 1.04f;
};

class Gui
{
public:
    Gui();
    ~Gui();

    Gui(const Gui&) = delete;
    Gui& operator=(const Gui&) = delete;

    bool Initialize(GLFWwindow* window);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    // Lighting settings for terrain
    struct LightingSettings
    {
        float sunIntensity = 1.0f;      // Overall sun brightness
        float ambientLight = 0.15f;     // Ambient light level
        float specularStrength = 0.0f;  // Specular highlight strength
        float specularPower = 32.0f;    // Specular shininess
    };

    // Consolidated panel methods
    void DrawScenePanel(
        glm::vec3& lightDir, float& sunSize, glm::vec3& sunColor,
        float& starDensity, LightingSettings& lighting);

    // Returns true if planet needs regeneration
    bool DrawTerrainPanel(
        bool& useGpu,
        EarthTerrainSettings& terrainSettings, uint32_t& seed, int& subdivisions,
        planets::core::Planet& planet,
        bool& useLodSystem, int& patchSubdivisions, float& planetRadius,
        int patchCount, int visiblePatchCount, int vertexCount,
        float cpuTimeMs, float gpuTimeMs, bool gpuAvailable);

    void DrawSurfacePanel(
        BiomeSettings& biomes, EarthColors& colors,
        EarthShadingSettings& shading,
        effects::OceanSettings& ocean, float& seaLevel);

    void DrawAtmospherePanel(effects::AtmosphereSettings& settings);

    void DrawDebugPanel(const planets::core::Camera& camera, float& moveSpeed);

private:
    void SetupDockspace();

    void DrawSpaceContent(glm::vec3& lightDir, float& sunSize, glm::vec3& sunColor, float& starDensity);
    void DrawLightingContent(LightingSettings& settings);

    bool DrawEarthTerrainContent(EarthTerrainSettings& settings, uint32_t& seed, int& subdivisions);
    bool DrawPlanetContent(planets::core::Planet& planet);
    bool DrawLodContent(bool& useLodSystem, int& patchSubdivisions, float& planetRadius,
        int patchCount, int visiblePatchCount, int vertexCount);
    void DrawGpuContent(bool& useGpu, float cpuTimeMs, float gpuTimeMs, bool gpuAvailable);

    void DrawBiomeContent(BiomeSettings& settings);
    void DrawEarthColorsContent(EarthColors& colors);
    void DrawShadingContent(EarthShadingSettings& settings);
    void DrawOceanContent(effects::OceanSettings& settings, float& seaLevel);

    void DrawAtmosphereContent(effects::AtmosphereSettings& settings);
    void DrawDebugContent(const planets::core::Camera& camera, float& moveSpeed);

    struct PanelVisibility {
        bool scene = true;
        bool terrain = true;
        bool surface = true;
        bool atmosphere = true;
        bool debug = true;
    };

    bool _initialized;
    bool _resetLayout;
    PanelVisibility _visibility;
};

} // namespace planets::render
