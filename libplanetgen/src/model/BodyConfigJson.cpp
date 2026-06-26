#include "BodyConfigJson.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

namespace planetgen
{

namespace
{

using json = nlohmann::json;

// Helper: read vec3 from [r, g, b] array
glm::vec3 ReadVec3(const json& j, const std::string& key, const glm::vec3& def)
{
    if (!j.contains(key))
        return def;
    const auto& a = j[key];
    if (!a.is_array() || a.size() < 3)
        return def;
    return { a[0].get<float>(), a[1].get<float>(), a[2].get<float>() };
}

json Vec3ToJson(const glm::vec3& v)
{
    return json::array({ v.x, v.y, v.z });
}

NoiseLayerConfig ParseNoiseLayer(const json& j, const NoiseLayerConfig& def)
{
    NoiseLayerConfig n = def;
    if (j.contains("scale"))       n.scale       = j["scale"].get<float>();
    if (j.contains("octaves"))     n.octaves     = j["octaves"].get<int>();
    if (j.contains("persistence")) n.persistence = j["persistence"].get<float>();
    if (j.contains("lacunarity"))  n.lacunarity  = j["lacunarity"].get<float>();
    if (j.contains("strength"))    n.strength    = j["strength"].get<float>();
    return n;
}

json NoiseLayerToJson(const NoiseLayerConfig& n)
{
    // Alphabetical order for determinism
    return {
        { "lacunarity",  n.lacunarity  },
        { "octaves",     n.octaves     },
        { "persistence", n.persistence },
        { "scale",       n.scale       },
        { "strength",    n.strength    },
    };
}

} // namespace

std::string BodyConfigFromJson(const std::string& jsonStr, BodyConfig& out)
{
    BodyConfig cfg; // default-initialised — all blocks carry per-block defaults

    try
    {
        auto j = json::parse(jsonStr);

        // --- Metadata ---
        if (j.contains("metadata"))
        {
            const auto& m = j["metadata"];
            cfg.metadata.name             = m.value("name", cfg.metadata.name);
            cfg.metadata.typeName         = m.value("typeName", cfg.metadata.typeName);
            cfg.metadata.radius           = m.value("radius", cfg.metadata.radius);
            cfg.metadata.seaLevel         = m.value("seaLevel", cfg.metadata.seaLevel);
            cfg.metadata.atmosphereHeight = m.value("atmosphereHeight", cfg.metadata.atmosphereHeight);
            cfg.metadata.hasSolidSurface  = m.value("hasSolidSurface", cfg.metadata.hasSolidSurface);
            cfg.metadata.hasAtmosphere    = m.value("hasAtmosphere", cfg.metadata.hasAtmosphere);
        }

        // --- Shape ---
        if (j.contains("shape"))
        {
            const auto& s = j["shape"];
            cfg.shape.heightScale          = s.value("heightScale", cfg.shape.heightScale);
            cfg.shape.oceanDepthMultiplier = s.value("oceanDepthMultiplier", cfg.shape.oceanDepthMultiplier);
            cfg.shape.oceanFloorDepth      = s.value("oceanFloorDepth", cfg.shape.oceanFloorDepth);
            cfg.shape.oceanFloorSmoothing  = s.value("oceanFloorSmoothing", cfg.shape.oceanFloorSmoothing);
            cfg.shape.mountainBlend        = s.value("mountainBlend", cfg.shape.mountainBlend);
            cfg.shape.continentBaseLevel   = s.value("continentBaseLevel", cfg.shape.continentBaseLevel);
            cfg.shape.globalFrequency      = s.value("globalFrequency", cfg.shape.globalFrequency);
            cfg.shape.normalEpsilon        = s.value("normalEpsilon", cfg.shape.normalEpsilon);
            cfg.shape.mountainPower        = s.value("mountainPower", cfg.shape.mountainPower);
            cfg.shape.mountainGain         = s.value("mountainGain", cfg.shape.mountainGain);
            cfg.shape.mountainSmoothing    = s.value("mountainSmoothing", cfg.shape.mountainSmoothing);
            if (s.contains("continentNoise"))
                cfg.shape.continentNoise = ParseNoiseLayer(s["continentNoise"], cfg.shape.continentNoise);
            if (s.contains("mountainNoise"))
                cfg.shape.mountainNoise = ParseNoiseLayer(s["mountainNoise"], cfg.shape.mountainNoise);
            if (s.contains("maskNoise"))
                cfg.shape.maskNoise = ParseNoiseLayer(s["maskNoise"], cfg.shape.maskNoise);
        }

        // --- Tectonics ---
        if (j.contains("tectonics"))
        {
            const auto& t = j["tectonics"];
            cfg.tectonics.enabled               = t.value("enabled", cfg.tectonics.enabled);
            cfg.tectonics.numPlates             = t.value("numPlates", cfg.tectonics.numPlates);
            cfg.tectonics.continentalFraction   = t.value("continentalFraction", cfg.tectonics.continentalFraction);
            cfg.tectonics.boundaryWidth         = t.value("boundaryWidth", cfg.tectonics.boundaryWidth);
            cfg.tectonics.convergentMountainScale = t.value("convergentMountainScale", cfg.tectonics.convergentMountainScale);
            cfg.tectonics.divergentRiftDepth    = t.value("divergentRiftDepth", cfg.tectonics.divergentRiftDepth);
            cfg.tectonics.coastlineNoise        = t.value("coastlineNoise", cfg.tectonics.coastlineNoise);
            cfg.tectonics.plateElevationNoise   = t.value("plateElevationNoise", cfg.tectonics.plateElevationNoise);
        }

        // --- OceanFloor ---
        if (j.contains("oceanFloor"))
        {
            const auto& o = j["oceanFloor"];
            cfg.oceanFloor.enabled           = o.value("enabled", cfg.oceanFloor.enabled);
            cfg.oceanFloor.shelfWidth        = o.value("shelfWidth", cfg.oceanFloor.shelfWidth);
            cfg.oceanFloor.oceanRidgeOctaves = o.value("oceanRidgeOctaves", cfg.oceanFloor.oceanRidgeOctaves);
            cfg.oceanFloor.oceanRidgeScale   = o.value("oceanRidgeScale", cfg.oceanFloor.oceanRidgeScale);
            cfg.oceanFloor.oceanRidgeStrength = o.value("oceanRidgeStrength", cfg.oceanFloor.oceanRidgeStrength);
            cfg.oceanFloor.oceanRidgePower   = o.value("oceanRidgePower", cfg.oceanFloor.oceanRidgePower);
            cfg.oceanFloor.oceanRidgeGain    = o.value("oceanRidgeGain", cfg.oceanFloor.oceanRidgeGain);
            cfg.oceanFloor.trenchOctaves     = o.value("trenchOctaves", cfg.oceanFloor.trenchOctaves);
            cfg.oceanFloor.trenchScale       = o.value("trenchScale", cfg.oceanFloor.trenchScale);
            cfg.oceanFloor.trenchDepth       = o.value("trenchDepth", cfg.oceanFloor.trenchDepth);
            cfg.oceanFloor.abyssalOctaves    = o.value("abyssalOctaves", cfg.oceanFloor.abyssalOctaves);
            cfg.oceanFloor.abyssalScale      = o.value("abyssalScale", cfg.oceanFloor.abyssalScale);
            cfg.oceanFloor.abyssalStrength   = o.value("abyssalStrength", cfg.oceanFloor.abyssalStrength);
        }

        // --- HeightDetail ---
        if (j.contains("heightDetail"))
        {
            const auto& h = j["heightDetail"];
            cfg.heightDetail.detailLowThreshold  = h.value("detailLowThreshold", cfg.heightDetail.detailLowThreshold);
            cfg.heightDetail.detailHighThreshold = h.value("detailHighThreshold", cfg.heightDetail.detailHighThreshold);
            cfg.heightDetail.perturbStrengthLow  = h.value("perturbStrengthLow", cfg.heightDetail.perturbStrengthLow);
            cfg.heightDetail.perturbStrengthHigh = h.value("perturbStrengthHigh", cfg.heightDetail.perturbStrengthHigh);
            cfg.heightDetail.detailOctavesLow    = h.value("detailOctavesLow", cfg.heightDetail.detailOctavesLow);
            cfg.heightDetail.detailOctavesHigh   = h.value("detailOctavesHigh", cfg.heightDetail.detailOctavesHigh);
            cfg.heightDetail.detailPersistence   = h.value("detailPersistence", cfg.heightDetail.detailPersistence);
            cfg.heightDetail.detailLacunarity    = h.value("detailLacunarity", cfg.heightDetail.detailLacunarity);
            cfg.heightDetail.perturbScale        = h.value("perturbScale", cfg.heightDetail.perturbScale);
        }

        // --- Shading ---
        if (j.contains("shading"))
        {
            const auto& s = j["shading"];
            cfg.shading.detailNoiseScale      = s.value("detailNoiseScale", cfg.shading.detailNoiseScale);
            cfg.shading.smallNoiseScale       = s.value("smallNoiseScale", cfg.shading.smallNoiseScale);
            cfg.shading.smallNoiseOctaves     = s.value("smallNoiseOctaves", cfg.shading.smallNoiseOctaves);
            cfg.shading.warpStrength          = s.value("warpStrength", cfg.shading.warpStrength);
            cfg.shading.useClimateModel       = s.value("useClimateModel", cfg.shading.useClimateModel);
            cfg.shading.largeNoiseScale       = s.value("largeNoiseScale", cfg.shading.largeNoiseScale);
            cfg.shading.largeNoiseOctaves     = s.value("largeNoiseOctaves", cfg.shading.largeNoiseOctaves);
            cfg.shading.temperatureLapseRate  = s.value("temperatureLapseRate", cfg.shading.temperatureLapseRate);
            cfg.shading.temperatureExponent   = s.value("temperatureExponent", cfg.shading.temperatureExponent);
            cfg.shading.moistureNoiseScale    = s.value("moistureNoiseScale", cfg.shading.moistureNoiseScale);
            cfg.shading.moistureNoiseStrength = s.value("moistureNoiseStrength", cfg.shading.moistureNoiseStrength);
            cfg.shading.hadleyIntensity       = s.value("hadleyIntensity", cfg.shading.hadleyIntensity);
            cfg.shading.continentalityStrength = s.value("continentalityStrength", cfg.shading.continentalityStrength);
            cfg.shading.biomesEnabled         = s.value("biomesEnabled", cfg.shading.biomesEnabled);
            cfg.shading.steepnessThreshold    = s.value("steepnessThreshold", cfg.shading.steepnessThreshold);
            cfg.shading.flatToSteepBlend      = s.value("flatToSteepBlend", cfg.shading.flatToSteepBlend);
            cfg.shading.snowLatitude          = s.value("snowLatitude", cfg.shading.snowLatitude);
            cfg.shading.snowBlend             = s.value("snowBlend", cfg.shading.snowBlend);
            cfg.shading.snowLine              = s.value("snowLine", cfg.shading.snowLine);
            cfg.shading.shoreHeight           = s.value("shoreHeight", cfg.shading.shoreHeight);
            cfg.shading.coastalDepthRange     = s.value("coastalDepthRange", cfg.shading.coastalDepthRange);
            cfg.shading.aoStrength            = s.value("aoStrength", cfg.shading.aoStrength);
            cfg.shading.flatColBlend          = s.value("flatColBlend", cfg.shading.flatColBlend);
            cfg.shading.flatColBlendNoise     = s.value("flatColBlendNoise", cfg.shading.flatColBlendNoise);
            cfg.shading.colorShoreLow  = ReadVec3(s, "colorShoreLow",  cfg.shading.colorShoreLow);
            cfg.shading.colorShoreHigh = ReadVec3(s, "colorShoreHigh", cfg.shading.colorShoreHigh);
            cfg.shading.colorFlatLowA  = ReadVec3(s, "colorFlatLowA",  cfg.shading.colorFlatLowA);
            cfg.shading.colorFlatHighA = ReadVec3(s, "colorFlatHighA", cfg.shading.colorFlatHighA);
            cfg.shading.colorFlatLowB  = ReadVec3(s, "colorFlatLowB",  cfg.shading.colorFlatLowB);
            cfg.shading.colorFlatHighB = ReadVec3(s, "colorFlatHighB", cfg.shading.colorFlatHighB);
            cfg.shading.colorSteepLow  = ReadVec3(s, "colorSteepLow",  cfg.shading.colorSteepLow);
            cfg.shading.colorSteepHigh = ReadVec3(s, "colorSteepHigh", cfg.shading.colorSteepHigh);
            cfg.shading.colorSnow      = ReadVec3(s, "colorSnow",      cfg.shading.colorSnow);
        }

        // --- Erosion ---
        if (j.contains("erosion"))
        {
            const auto& e = j["erosion"];
            cfg.erosion.enabled         = e.value("enabled", cfg.erosion.enabled);
            cfg.erosion.iterations      = e.value("iterations", cfg.erosion.iterations);
            cfg.erosion.gridResolution  = e.value("gridResolution", cfg.erosion.gridResolution);
            cfg.erosion.thermalRate     = e.value("thermalRate", cfg.erosion.thermalRate);
            cfg.erosion.thermalThreshold = e.value("thermalThreshold", cfg.erosion.thermalThreshold);
            cfg.erosion.hydraulicRate   = e.value("hydraulicRate", cfg.erosion.hydraulicRate);
            cfg.erosion.depositionRate  = e.value("depositionRate", cfg.erosion.depositionRate);
            cfg.erosion.evaporationRate = e.value("evaporationRate", cfg.erosion.evaporationRate);
        }

        // --- PaletteRef ---
        if (j.contains("paletteRef"))
        {
            const auto& p = j["paletteRef"];
            cfg.paletteRef.paletteId = p.value("paletteId", cfg.paletteRef.paletteId);
        }

        // --- ShaderPaths ---
        if (j.contains("shaderPaths"))
        {
            const auto& sp = j["shaderPaths"];
            cfg.shaderPaths.heightShaderPath   = sp.value("height",   cfg.shaderPaths.heightShaderPath);
            cfg.shaderPaths.shadingShaderPath  = sp.value("shading",  cfg.shaderPaths.shadingShaderPath);
            cfg.shaderPaths.erosionShaderPath  = sp.value("erosion",  cfg.shaderPaths.erosionShaderPath);
            cfg.shaderPaths.vertexShaderPath   = sp.value("vertex",   cfg.shaderPaths.vertexShaderPath);
            cfg.shaderPaths.fragmentShaderPath = sp.value("fragment", cfg.shaderPaths.fragmentShaderPath);
        }
    }
    catch (const json::exception& e)
    {
        return std::string("JSON parse error: ") + e.what();
    }

    // Validate after filling defaults
    std::string reason = cfg.Validate();
    if (!reason.empty())
        return reason;

    out = std::move(cfg);
    return {};
}

std::string BodyConfigToJson(const BodyConfig& cfg)
{
    // Alphabetical key order within each block for determinism
    json j;

    j["erosion"] = {
        { "depositionRate",  cfg.erosion.depositionRate  },
        { "enabled",         cfg.erosion.enabled         },
        { "evaporationRate", cfg.erosion.evaporationRate },
        { "gridResolution",  cfg.erosion.gridResolution  },
        { "hydraulicRate",   cfg.erosion.hydraulicRate   },
        { "iterations",      cfg.erosion.iterations      },
        { "thermalRate",     cfg.erosion.thermalRate     },
        { "thermalThreshold", cfg.erosion.thermalThreshold },
    };

    j["heightDetail"] = {
        { "detailHighThreshold", cfg.heightDetail.detailHighThreshold },
        { "detailLacunarity",    cfg.heightDetail.detailLacunarity    },
        { "detailLowThreshold",  cfg.heightDetail.detailLowThreshold  },
        { "detailOctavesHigh",   cfg.heightDetail.detailOctavesHigh   },
        { "detailOctavesLow",    cfg.heightDetail.detailOctavesLow    },
        { "detailPersistence",   cfg.heightDetail.detailPersistence   },
        { "perturbScale",        cfg.heightDetail.perturbScale        },
        { "perturbStrengthHigh", cfg.heightDetail.perturbStrengthHigh },
        { "perturbStrengthLow",  cfg.heightDetail.perturbStrengthLow  },
    };

    j["metadata"] = {
        { "atmosphereHeight", cfg.metadata.atmosphereHeight },
        { "hasAtmosphere",    cfg.metadata.hasAtmosphere    },
        { "hasSolidSurface",  cfg.metadata.hasSolidSurface  },
        { "name",             cfg.metadata.name             },
        { "radius",           cfg.metadata.radius           },
        { "seaLevel",         cfg.metadata.seaLevel         },
        { "typeName",         cfg.metadata.typeName         },
    };

    j["oceanFloor"] = {
        { "abyssalOctaves",    cfg.oceanFloor.abyssalOctaves    },
        { "abyssalScale",      cfg.oceanFloor.abyssalScale      },
        { "abyssalStrength",   cfg.oceanFloor.abyssalStrength   },
        { "enabled",           cfg.oceanFloor.enabled           },
        { "oceanRidgeGain",    cfg.oceanFloor.oceanRidgeGain    },
        { "oceanRidgeOctaves", cfg.oceanFloor.oceanRidgeOctaves },
        { "oceanRidgePower",   cfg.oceanFloor.oceanRidgePower   },
        { "oceanRidgeScale",   cfg.oceanFloor.oceanRidgeScale   },
        { "oceanRidgeStrength", cfg.oceanFloor.oceanRidgeStrength },
        { "shelfWidth",        cfg.oceanFloor.shelfWidth        },
        { "trenchDepth",       cfg.oceanFloor.trenchDepth       },
        { "trenchOctaves",     cfg.oceanFloor.trenchOctaves     },
        { "trenchScale",       cfg.oceanFloor.trenchScale       },
    };

    j["paletteRef"] = { { "paletteId", cfg.paletteRef.paletteId } };

    j["shaderPaths"] = {
        { "erosion",  cfg.shaderPaths.erosionShaderPath  },
        { "fragment", cfg.shaderPaths.fragmentShaderPath },
        { "height",   cfg.shaderPaths.heightShaderPath   },
        { "shading",  cfg.shaderPaths.shadingShaderPath  },
        { "vertex",   cfg.shaderPaths.vertexShaderPath   },
    };

    j["shading"] = {
        { "aoStrength",            cfg.shading.aoStrength            },
        { "biomesEnabled",         cfg.shading.biomesEnabled         },
        { "coastalDepthRange",     cfg.shading.coastalDepthRange     },
        { "colorFlatHighA",        Vec3ToJson(cfg.shading.colorFlatHighA) },
        { "colorFlatHighB",        Vec3ToJson(cfg.shading.colorFlatHighB) },
        { "colorFlatLowA",         Vec3ToJson(cfg.shading.colorFlatLowA)  },
        { "colorFlatLowB",         Vec3ToJson(cfg.shading.colorFlatLowB)  },
        { "colorShoreHigh",        Vec3ToJson(cfg.shading.colorShoreHigh) },
        { "colorShoreLow",         Vec3ToJson(cfg.shading.colorShoreLow)  },
        { "colorSnow",             Vec3ToJson(cfg.shading.colorSnow)      },
        { "colorSteepHigh",        Vec3ToJson(cfg.shading.colorSteepHigh) },
        { "colorSteepLow",         Vec3ToJson(cfg.shading.colorSteepLow)  },
        { "continentalityStrength", cfg.shading.continentalityStrength   },
        { "detailNoiseScale",      cfg.shading.detailNoiseScale           },
        { "flatColBlend",          cfg.shading.flatColBlend              },
        { "flatColBlendNoise",     cfg.shading.flatColBlendNoise         },
        { "flatToSteepBlend",      cfg.shading.flatToSteepBlend          },
        { "hadleyIntensity",       cfg.shading.hadleyIntensity           },
        { "largeNoiseOctaves",     cfg.shading.largeNoiseOctaves         },
        { "largeNoiseScale",       cfg.shading.largeNoiseScale           },
        { "moistureNoiseScale",    cfg.shading.moistureNoiseScale        },
        { "moistureNoiseStrength", cfg.shading.moistureNoiseStrength     },
        { "shoreHeight",           cfg.shading.shoreHeight               },
        { "smallNoiseOctaves",     cfg.shading.smallNoiseOctaves         },
        { "smallNoiseScale",       cfg.shading.smallNoiseScale           },
        { "snowBlend",             cfg.shading.snowBlend                 },
        { "snowLatitude",          cfg.shading.snowLatitude              },
        { "snowLine",              cfg.shading.snowLine                  },
        { "steepnessThreshold",    cfg.shading.steepnessThreshold        },
        { "temperatureExponent",   cfg.shading.temperatureExponent       },
        { "temperatureLapseRate",  cfg.shading.temperatureLapseRate      },
        { "useClimateModel",       cfg.shading.useClimateModel           },
        { "warpStrength",          cfg.shading.warpStrength              },
    };

    j["shape"] = {
        { "continentBaseLevel",   cfg.shape.continentBaseLevel   },
        { "continentNoise",       NoiseLayerToJson(cfg.shape.continentNoise) },
        { "globalFrequency",      cfg.shape.globalFrequency      },
        { "heightScale",          cfg.shape.heightScale          },
        { "maskNoise",            NoiseLayerToJson(cfg.shape.maskNoise)      },
        { "mountainBlend",        cfg.shape.mountainBlend        },
        { "mountainGain",         cfg.shape.mountainGain         },
        { "mountainNoise",        NoiseLayerToJson(cfg.shape.mountainNoise)  },
        { "mountainPower",        cfg.shape.mountainPower        },
        { "mountainSmoothing",    cfg.shape.mountainSmoothing    },
        { "normalEpsilon",        cfg.shape.normalEpsilon        },
        { "oceanDepthMultiplier", cfg.shape.oceanDepthMultiplier },
        { "oceanFloorDepth",      cfg.shape.oceanFloorDepth      },
        { "oceanFloorSmoothing",  cfg.shape.oceanFloorSmoothing  },
    };

    j["tectonics"] = {
        { "boundaryWidth",          cfg.tectonics.boundaryWidth          },
        { "coastlineNoise",         cfg.tectonics.coastlineNoise         },
        { "continentalFraction",    cfg.tectonics.continentalFraction    },
        { "convergentMountainScale", cfg.tectonics.convergentMountainScale },
        { "divergentRiftDepth",     cfg.tectonics.divergentRiftDepth     },
        { "enabled",                cfg.tectonics.enabled                },
        { "numPlates",              cfg.tectonics.numPlates              },
        { "plateElevationNoise",    cfg.tectonics.plateElevationNoise    },
    };

    return j.dump(4);
}

std::string BodyConfigLoadFile(const std::string& path, BodyConfig& out)
{
    std::ifstream f(path);
    if (!f.is_open())
        return "failed to open file: " + path;

    std::ostringstream ss;
    ss << f.rdbuf();
    return BodyConfigFromJson(ss.str(), out);
}

} // namespace planetgen
