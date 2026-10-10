// Shared std140 parameter block for every macro world bake pass.
// Matches MacroParams in ParamBlocks.h exactly (field order and padding).
// Elevations inside the bake are meters (elevation units * metersPerUnit) so the
// ported erosion constants keep their physical meaning. Sea level is 0.

layout(std140, binding = 3) uniform MacroParamsBlock
{
    // Row 0
    uint  resolution;
    uint  seed;
    int   numPlates;
    float boundaryWidth;
    // Row 1
    float convergentThreshold;
    float convergentMountainScale;
    float divergentRiftDepth;
    float plateElevationNoise;
    // Row 2
    float continentScale;      // already multiplied by globalFrequency
    int   continentLayers;
    float continentPersistence;
    float continentLacunarity;
    // Row 3
    vec3  continentOffset;
    float continentBaseLevel;
    // Row 4
    float rainRate;
    float evaporationRate;
    float flowRate;
    float erosionRate;
    // Row 5
    float depositionRate;
    float capacityMultiplier;
    float cellSizeMeters;
    float metersPerUnit;
    // Row 6
    float windStrength;
    float landDrying;
    float orographicStrength;
    float temperatureLapseRate;
    // Row 7
    float temperatureExponent;
    float channelFluxThreshold;
    uint  passIndex;
    float _macroPad0;
};
