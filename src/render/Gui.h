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

    void DrawDebugPanel(const planets::core::Camera& camera, float& moveSpeed);

    // Returns true if planet needs regeneration
    bool DrawPlanetPanel(planets::core::Planet& planet);

    // GPU compute panel with timing info
    void DrawGpuPanel(bool& useGpu, float cpuTimeMs, float gpuTimeMs, bool gpuAvailable);

    // Earth terrain generation panel
    // Returns true if terrain needs regeneration
    bool DrawEarthTerrainPanel(EarthTerrainSettings& settings, uint32_t& seed, int& subdivisions);

    // LOD system panel
    // Returns true if LOD system needs regeneration
    bool DrawLodPanel(
        bool& useLodSystem,
        int& patchSubdivisions,
        float& planetRadius,
        int patchCount,
        int visiblePatchCount,
        int vertexCount);

    // Ocean settings panel
    void DrawOceanPanel(effects::OceanSettings& settings, float& seaLevel);

    // Atmosphere settings panel
    void DrawAtmospherePanel(effects::AtmosphereSettings& settings);

    // Biome settings panel
    void DrawBiomePanel(BiomeSettings& settings);

    // Earth colors panel (gradient pairs for terrain)
    void DrawEarthColorsPanel(EarthColors& colors);

    // Shading settings panel
    void DrawShadingPanel(EarthShadingSettings& settings);

    // Lighting settings for terrain
    struct LightingSettings
    {
        float sunIntensity = 1.0f;      // Overall sun brightness
        float ambientLight = 0.15f;     // Ambient light level
        float specularStrength = 0.0f;  // Specular highlight strength
        float specularPower = 32.0f;    // Specular shininess
    };

    // Space/sun settings panel
    void DrawSpacePanel(glm::vec3& lightDir, float& sunSize, glm::vec3& sunColor, float& starDensity);

    // Lighting settings panel
    void DrawLightingPanel(LightingSettings& settings);

private:
    bool _initialized;
};

} // namespace planets::render
