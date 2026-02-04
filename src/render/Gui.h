#pragma once

#include <cstdint>

struct GLFWwindow;

namespace planets::core {
class Camera;
class Planet;
}

namespace planets::render {

struct EarthTerrainSettings;

namespace effects {
struct OceanSettings;
struct AtmosphereSettings;
}

// Biome settings for shader
struct BiomeSettings
{
    bool enabled = true;
    float temperatureScale = 0.5f;
    float humidityScale = 0.7f;
    float latitudeInfluence = 0.6f;
    float snowLine = 0.7f;
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

private:
    bool _initialized;
};

} // namespace planets::render
