#include "BodyConfig.h"
#include <sstream>

namespace planetgen
{

std::string BodyConfig::Validate() const
{
    std::ostringstream err;

    // Metadata
    if (metadata.radius <= 0.0f)
        err << "metadata.radius must be > 0; ";
    if (metadata.seaLevel < 0.0f || metadata.seaLevel > 2.0f)
        err << "metadata.seaLevel out of [0, 2]; ";
    if (metadata.atmosphereHeight < 0.0f)
        err << "metadata.atmosphereHeight must be >= 0; ";

    // Shape
    if (shape.heightScale <= 0.0f || shape.heightScale > 1.0f)
        err << "shape.heightScale out of (0, 1]; ";
    if (shape.globalFrequency <= 0.0f)
        err << "shape.globalFrequency must be > 0; ";
    if (shape.normalEpsilon <= 0.0f)
        err << "shape.normalEpsilon must be > 0; ";
    if (shape.continentNoise.octaves < 1 || shape.continentNoise.octaves > 16)
        err << "shape.continentNoise.octaves out of [1,16]; ";
    if (shape.mountainNoise.octaves < 1 || shape.mountainNoise.octaves > 16)
        err << "shape.mountainNoise.octaves out of [1,16]; ";
    if (shape.maskNoise.octaves < 1 || shape.maskNoise.octaves > 16)
        err << "shape.maskNoise.octaves out of [1,16]; ";

    // Tectonics — only validate field values when enabled
    if (tectonics.enabled)
    {
        if (tectonics.numPlates < 2 || tectonics.numPlates > 64)
            err << "tectonics.numPlates out of [2, 64]; ";
        if (tectonics.continentalFraction < 0.0f || tectonics.continentalFraction > 1.0f)
            err << "tectonics.continentalFraction out of [0, 1]; ";
        if (tectonics.boundaryWidth < 0.0f || tectonics.boundaryWidth > 1.0f)
            err << "tectonics.boundaryWidth out of [0, 1]; ";
    }

    // OceanFloor — only validate field values when enabled
    if (oceanFloor.enabled)
    {
        if (oceanFloor.shelfWidth < 0.0f || oceanFloor.shelfWidth > 1.0f)
            err << "oceanFloor.shelfWidth out of [0, 1]; ";
        if (oceanFloor.oceanRidgeOctaves < 1 || oceanFloor.oceanRidgeOctaves > 16)
            err << "oceanFloor.oceanRidgeOctaves out of [1, 16]; ";
        if (oceanFloor.trenchDepth < 0.0f || oceanFloor.trenchDepth > 5.0f)
            err << "oceanFloor.trenchDepth out of [0, 5]; ";
    }

    // HeightDetail
    if (heightDetail.detailOctavesLow < 0 || heightDetail.detailOctavesLow > 16)
        err << "heightDetail.detailOctavesLow out of [0,16]; ";
    if (heightDetail.detailOctavesHigh < 0 || heightDetail.detailOctavesHigh > 16)
        err << "heightDetail.detailOctavesHigh out of [0,16]; ";
    if (heightDetail.detailLowThreshold > heightDetail.detailHighThreshold)
        err << "heightDetail.detailLowThreshold must be <= detailHighThreshold; ";

    // Shading
    if (shading.smallNoiseOctaves < 1 || shading.smallNoiseOctaves > 16)
        err << "shading.smallNoiseOctaves out of [1, 16]; ";
    if (shading.useClimateModel)
    {
        if (shading.hadleyIntensity < 0.0f)
            err << "shading.hadleyIntensity must be >= 0 when climate model enabled; ";
    }

    // Erosion — only validate field values when enabled
    if (erosion.enabled)
    {
        if (erosion.iterations < 1 || erosion.iterations > 1000)
            err << "erosion.iterations out of [1, 1000]; ";
        if (erosion.gridResolution < 4 || erosion.gridResolution > 4096)
            err << "erosion.gridResolution out of [4, 4096]; ";
        if (erosion.thermalRate < 0.0f || erosion.thermalRate > 1.0f)
            err << "erosion.thermalRate out of [0, 1]; ";
        if (erosion.hydraulicRate < 0.0f || erosion.hydraulicRate > 1.0f)
            err << "erosion.hydraulicRate out of [0, 1]; ";
        if (erosion.evaporationRate < 0.0f || erosion.evaporationRate > 1.0f)
            err << "erosion.evaporationRate out of [0, 1]; ";
    }

    // MacroWorld â€” only validate field values when enabled
    if (macroWorld.enabled)
    {
        const auto& m = macroWorld;
        if (m.resolution < 32 || m.resolution > 1024)
            err << "macroWorld.resolution out of [32, 1024]; ";
        if (m.convergentThreshold < -1.0f || m.convergentThreshold > 1.0f)
            err << "macroWorld.convergentThreshold out of [-1, 1]; ";
        if (m.erosionIterations < 0 || m.erosionIterations > 1000)
            err << "macroWorld.erosionIterations out of [0, 1000]; ";
        if (m.rainRate < 0.0f || m.rainRate > 1.0f)
            err << "macroWorld.rainRate out of [0, 1]; ";
        if (m.evaporationRate < 0.0f || m.evaporationRate > 1.0f)
            err << "macroWorld.evaporationRate out of [0, 1]; ";
        if (m.flowRate < 0.0f || m.flowRate > 1.0f)
            err << "macroWorld.flowRate out of [0, 1]; ";
        if (m.erosionRate < 0.0f || m.erosionRate > 1.0f)
            err << "macroWorld.erosionRate out of [0, 1]; ";
        if (m.depositionRate < 0.0f || m.depositionRate > 1.0f)
            err << "macroWorld.depositionRate out of [0, 1]; ";
        if (m.capacityMultiplier < 0.0f || m.capacityMultiplier > 100.0f)
            err << "macroWorld.capacityMultiplier out of [0, 100]; ";
        if (m.moistureIterations < 0 || m.moistureIterations > 512)
            err << "macroWorld.moistureIterations out of [0, 512]; ";
        if (m.windStrength <= 0.0f || m.windStrength > 8.0f)
            err << "macroWorld.windStrength out of (0, 8]; ";
        if (m.landDrying < 0.0f || m.landDrying > 1.0f)
            err << "macroWorld.landDrying out of [0, 1]; ";
        if (m.orographicStrength < 0.0f || m.orographicStrength > 100.0f)
            err << "macroWorld.orographicStrength out of [0, 100]; ";
    }

    // PaletteRef
    if (paletteRef.paletteId.empty())
        err << "paletteRef.paletteId must not be empty; ";

    return err.str();
}

} // namespace planetgen
