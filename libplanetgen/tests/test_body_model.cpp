#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "model/BodyConfig.h"
#include "model/BodyConfigProjection.h"
#include "model/BodyConfigJson.h"
#include "model/PaletteRegistry.h"
#include "backend/ParamBlocks.h"
#include "planetgen/planetgen.h"

#include <cstring>
#include <cmath>

// ============================================================================
// (a) Default BodyConfig projects byte-equal to pg_body_desc_defaults
// ============================================================================

TEST_CASE("Default BodyConfig projects to PgBodyDesc equal to pg_body_desc_defaults", "[model][projection]")
{
    planetgen::BodyConfig defaultCfg;
    PgBodyDesc projected = planetgen::ProjectToBodyDesc(defaultCfg);

    PgBodyDesc reference{};
    pg_body_desc_defaults(&reference);

    // Compare every field individually (struct-level memcmp would work but field checks are clearer)
    CHECK(projected.continent_noise.scale       == reference.continent_noise.scale);
    CHECK(projected.continent_noise.octaves     == reference.continent_noise.octaves);
    CHECK(projected.continent_noise.persistence == reference.continent_noise.persistence);
    CHECK(projected.continent_noise.lacunarity  == reference.continent_noise.lacunarity);
    CHECK(projected.continent_noise.strength    == reference.continent_noise.strength);

    CHECK(projected.mountain_noise.scale       == reference.mountain_noise.scale);
    CHECK(projected.mountain_noise.octaves     == reference.mountain_noise.octaves);
    CHECK(projected.mountain_noise.persistence == reference.mountain_noise.persistence);
    CHECK(projected.mountain_noise.lacunarity  == reference.mountain_noise.lacunarity);
    CHECK(projected.mountain_noise.strength    == reference.mountain_noise.strength);

    CHECK(projected.mask_noise.scale       == reference.mask_noise.scale);
    CHECK(projected.mask_noise.octaves     == reference.mask_noise.octaves);
    CHECK(projected.mask_noise.persistence == reference.mask_noise.persistence);
    CHECK(projected.mask_noise.lacunarity  == reference.mask_noise.lacunarity);
    CHECK(projected.mask_noise.strength    == reference.mask_noise.strength);

    CHECK(projected.height_scale           == reference.height_scale);
    CHECK(projected.ocean_depth_multiplier == reference.ocean_depth_multiplier);
    CHECK(projected.ocean_floor_depth      == reference.ocean_floor_depth);
    CHECK(projected.ocean_floor_smoothing  == reference.ocean_floor_smoothing);
    CHECK(projected.mountain_blend         == reference.mountain_blend);
    CHECK(projected.continent_base_level   == reference.continent_base_level);
    CHECK(projected.global_frequency       == reference.global_frequency);
    CHECK(projected.normal_epsilon         == reference.normal_epsilon);
    CHECK(projected.mountain_power         == reference.mountain_power);
    CHECK(projected.mountain_gain          == reference.mountain_gain);
    CHECK(projected.mountain_smoothing     == reference.mountain_smoothing);

    CHECK(projected.use_tectonics            == reference.use_tectonics);
    CHECK(projected.num_plates               == reference.num_plates);
    CHECK(projected.continental_fraction     == reference.continental_fraction);
    CHECK(projected.boundary_width           == reference.boundary_width);
    CHECK(projected.convergent_mountain_scale == reference.convergent_mountain_scale);
    CHECK(projected.divergent_rift_depth     == reference.divergent_rift_depth);
    CHECK(projected.coastline_noise          == reference.coastline_noise);
    CHECK(projected.plate_elevation_noise    == reference.plate_elevation_noise);

    CHECK(projected.use_ocean_floor      == reference.use_ocean_floor);
    CHECK(projected.shelf_width          == reference.shelf_width);
    CHECK(projected.ocean_ridge_octaves  == reference.ocean_ridge_octaves);
    CHECK(projected.ocean_ridge_scale    == reference.ocean_ridge_scale);
    CHECK(projected.ocean_ridge_strength == reference.ocean_ridge_strength);
    CHECK(projected.ocean_ridge_power    == reference.ocean_ridge_power);
    CHECK(projected.ocean_ridge_gain     == reference.ocean_ridge_gain);
    CHECK(projected.trench_octaves       == reference.trench_octaves);
    CHECK(projected.trench_scale         == reference.trench_scale);
    CHECK(projected.trench_depth         == reference.trench_depth);
    CHECK(projected.abyssal_octaves      == reference.abyssal_octaves);
    CHECK(projected.abyssal_scale        == reference.abyssal_scale);
    CHECK(projected.abyssal_strength     == reference.abyssal_strength);

    CHECK(projected.detail_low_threshold  == reference.detail_low_threshold);
    CHECK(projected.detail_high_threshold == reference.detail_high_threshold);
    CHECK(projected.perturb_strength_low  == reference.perturb_strength_low);
    CHECK(projected.perturb_strength_high == reference.perturb_strength_high);
    CHECK(projected.detail_octaves_low    == reference.detail_octaves_low);
    CHECK(projected.detail_octaves_high   == reference.detail_octaves_high);
    CHECK(projected.detail_persistence    == reference.detail_persistence);
    CHECK(projected.detail_lacunarity     == reference.detail_lacunarity);
    CHECK(projected.perturb_scale         == reference.perturb_scale);
}

TEST_CASE("Default BodyConfig projects PgShadingDesc equal to pg_shading_desc_defaults", "[model][projection]")
{
    planetgen::BodyConfig defaultCfg;
    PgShadingDesc projected = planetgen::ProjectToShadingDesc(defaultCfg);

    PgShadingDesc reference{};
    pg_shading_desc_defaults(&reference);

    CHECK(projected.detail_noise_scale      == reference.detail_noise_scale);
    CHECK(projected.small_noise_scale       == reference.small_noise_scale);
    CHECK(projected.small_noise_octaves     == reference.small_noise_octaves);
    CHECK(projected.warp_strength           == reference.warp_strength);
    CHECK(projected.use_climate_model       == reference.use_climate_model);
    CHECK(projected.large_noise_scale       == reference.large_noise_scale);
    CHECK(projected.large_noise_octaves     == reference.large_noise_octaves);
    CHECK(projected.temperature_lapse_rate  == reference.temperature_lapse_rate);
    CHECK(projected.temperature_exponent    == reference.temperature_exponent);
    CHECK(projected.moisture_noise_scale    == reference.moisture_noise_scale);
    CHECK(projected.moisture_noise_strength == reference.moisture_noise_strength);
    CHECK(projected.hadley_intensity        == reference.hadley_intensity);
    CHECK(projected.continentality_strength == reference.continentality_strength);
    CHECK(projected.height_scale            == reference.height_scale);
}

TEST_CASE("Default BodyConfig projects PgErosionDesc equal to pg_erosion_desc_defaults", "[model][projection]")
{
    planetgen::BodyConfig defaultCfg;
    PgErosionDesc projected = planetgen::ProjectToErosionDesc(defaultCfg);

    PgErosionDesc reference{};
    pg_erosion_desc_defaults(&reference);

    CHECK(projected.iterations       == reference.iterations);
    CHECK(projected.grid_resolution  == reference.grid_resolution);
    CHECK(projected.thermal_rate     == reference.thermal_rate);
    CHECK(projected.thermal_threshold == reference.thermal_threshold);
    CHECK(projected.hydraulic_rate   == reference.hydraulic_rate);
    CHECK(projected.deposition_rate  == reference.deposition_rate);
    CHECK(projected.evaporation_rate == reference.evaporation_rate);
}

// ============================================================================
// (b) JSON round-trip equality
// ============================================================================

TEST_CASE("BodyConfig JSON round-trip: serialize then parse yields equal config", "[model][json]")
{
    planetgen::BodyConfig original;
    // Modify a few fields to make the round-trip non-trivial
    original.shape.heightScale     = 0.07f;
    original.shape.mountainBlend   = 0.99f;
    original.tectonics.numPlates   = 8;
    original.tectonics.enabled     = false;
    original.metadata.name         = "TestPlanet";
    original.paletteRef.paletteId  = "earth";

    std::string json = planetgen::BodyConfigToJson(original);
    REQUIRE(!json.empty());

    planetgen::BodyConfig parsed;
    std::string err = planetgen::BodyConfigFromJson(json, parsed);
    REQUIRE(err.empty());

    CHECK(parsed.shape.heightScale     == original.shape.heightScale);
    CHECK(parsed.shape.mountainBlend   == original.shape.mountainBlend);
    CHECK(parsed.tectonics.numPlates   == original.tectonics.numPlates);
    CHECK(parsed.tectonics.enabled     == original.tectonics.enabled);
    CHECK(parsed.metadata.name         == original.metadata.name);
    CHECK(parsed.paletteRef.paletteId  == original.paletteRef.paletteId);
    CHECK(parsed.shading.useClimateModel == original.shading.useClimateModel);
    CHECK(parsed.erosion.enabled       == original.erosion.enabled);
}

// ============================================================================
// (c) Negative test — out-of-range / inconsistent config rejected with reason
// ============================================================================

TEST_CASE("Validation rejects out-of-range heightScale", "[model][validation]")
{
    planetgen::BodyConfig cfg;
    cfg.shape.heightScale = -1.0f; // invalid
    std::string reason = cfg.Validate();
    REQUIRE(!reason.empty());
    INFO("Reason: " << reason);
}

TEST_CASE("Validation rejects radius <= 0", "[model][validation]")
{
    planetgen::BodyConfig cfg;
    cfg.metadata.radius = 0.0f;
    std::string reason = cfg.Validate();
    REQUIRE(!reason.empty());
    INFO("Reason: " << reason);
}

TEST_CASE("Validation rejects inverted detail thresholds", "[model][validation]")
{
    planetgen::BodyConfig cfg;
    cfg.heightDetail.detailLowThreshold  = 0.5f;
    cfg.heightDetail.detailHighThreshold = 0.1f; // low > high — invalid
    std::string reason = cfg.Validate();
    REQUIRE(!reason.empty());
    INFO("Reason: " << reason);
}

TEST_CASE("Validation rejects out-of-range tectonics when enabled", "[model][validation]")
{
    planetgen::BodyConfig cfg;
    cfg.tectonics.enabled   = true;
    cfg.tectonics.numPlates = 100; // > 64 — invalid
    std::string reason = cfg.Validate();
    REQUIRE(!reason.empty());
    INFO("Reason: " << reason);
}

TEST_CASE("Validation accepts disabled tectonics with out-of-range fields", "[model][validation]")
{
    // When disabled, field values are ignored during validation
    planetgen::BodyConfig cfg;
    cfg.tectonics.enabled   = false;
    cfg.tectonics.numPlates = 100; // would be invalid if enabled
    std::string reason = cfg.Validate();
    REQUIRE(reason.empty()); // should pass since tectonics disabled
}

TEST_CASE("JSON parse rejects config with invalid heightScale", "[model][json][validation]")
{
    std::string json = R"({
        "shape": { "heightScale": 5.0 }
    })";
    planetgen::BodyConfig cfg;
    std::string err = planetgen::BodyConfigFromJson(json, cfg);
    REQUIRE(!err.empty());
    INFO("Error: " << err);
}

// ============================================================================
// (d) Custom palette id register-and-resolve
// ============================================================================

TEST_CASE("PaletteRegistry registers and resolves built-in palettes", "[model][palette]")
{
    planetgen::PaletteRegistry reg;

    REQUIRE(reg.Contains("earth"));
    REQUIRE(reg.Contains("volcanic"));
    REQUIRE(reg.Contains("crystalline"));

    const auto& earth = reg.Resolve("earth");
    REQUIRE(earth.IsValid());
    CHECK(earth.name == "Earth");
    CHECK(earth.entries.size() == 11);

    const auto& volcanic = reg.Resolve("volcanic");
    REQUIRE(volcanic.IsValid());
    CHECK(volcanic.entries.size() == 7);

    const auto& crystalline = reg.Resolve("crystalline");
    REQUIRE(crystalline.IsValid());
    CHECK(crystalline.entries.size() == 7);
}

TEST_CASE("PaletteRegistry custom id register and resolve", "[model][palette]")
{
    planetgen::PaletteRegistry reg;

    planetgen::Palette custom;
    custom.name = "MyPlanet";
    custom.entries = {
        { { 1.0f, 0.0f, 0.0f }, 0.0f },
        { { 0.0f, 1.0f, 0.0f }, 0.5f },
        { { 0.0f, 0.0f, 1.0f }, 1.0f },
    };

    REQUIRE_FALSE(reg.Contains("my_planet"));
    reg.Register("my_planet", custom);
    REQUIRE(reg.Contains("my_planet"));

    const auto& resolved = reg.Resolve("my_planet");
    REQUIRE(resolved.IsValid());
    CHECK(resolved.name == "MyPlanet");
    CHECK(resolved.entries.size() == 3);
    CHECK(resolved.entries[1].parameter == Catch::Approx(0.5f));
}

TEST_CASE("PaletteRegistry resolve unknown id returns empty palette", "[model][palette]")
{
    planetgen::PaletteRegistry reg;
    const auto& p = reg.Resolve("nonexistent");
    CHECK_FALSE(p.IsValid());
}

// ============================================================================
// (e) Parity: earth/volcanic/crystalline JSON load + validate clean
// ============================================================================

TEST_CASE("earth.json loads and validates cleanly", "[model][parity]")
{
    planetgen::BodyConfig cfg;
    std::string err = planetgen::BodyConfigLoadFile("data/bodies/earth.json", cfg);
    REQUIRE(err.empty());
    CHECK(cfg.metadata.name == "Earth");
    CHECK(cfg.tectonics.enabled == true);
    CHECK(cfg.oceanFloor.enabled == true);
    CHECK(cfg.paletteRef.paletteId == "earth");
    // Verify key Earth values match expected defaults
    CHECK(cfg.shape.heightScale     == Catch::Approx(0.04f));
    CHECK(cfg.shape.mountainBlend   == Catch::Approx(1.16f));
    CHECK(cfg.tectonics.numPlates   == 12);
}

TEST_CASE("volcanic.json loads and validates cleanly", "[model][parity]")
{
    planetgen::BodyConfig cfg;
    std::string err = planetgen::BodyConfigLoadFile("data/bodies/volcanic.json", cfg);
    REQUIRE(err.empty());
    CHECK(cfg.metadata.name == "Volcanic World");
    CHECK(cfg.tectonics.enabled == false);
    CHECK(cfg.oceanFloor.enabled == false);
    CHECK(cfg.paletteRef.paletteId == "volcanic");
    CHECK(cfg.shape.heightScale == Catch::Approx(0.06f));
}

TEST_CASE("crystalline.json loads and validates cleanly", "[model][parity]")
{
    planetgen::BodyConfig cfg;
    std::string err = planetgen::BodyConfigLoadFile("data/bodies/crystalline.json", cfg);
    REQUIRE(err.empty());
    CHECK(cfg.metadata.name == "Crystalline World");
    CHECK(cfg.tectonics.enabled == false);
    CHECK(cfg.paletteRef.paletteId == "crystalline");
    CHECK(cfg.shape.heightScale == Catch::Approx(0.08f));
}

// ============================================================================
// Continental mask growth params — Step 1/3 of the continent mask feature
// ============================================================================

TEST_CASE("BodyConfig has continent mask growth fields with correct defaults", "[model][continent-mask]")
{
    planetgen::BodyConfig cfg;
    CHECK(cfg.shape.continentCount          == 6);
    CHECK(cfg.shape.continentSizeVariance   == Catch::Approx(0.4f));
    CHECK(cfg.shape.continentClustering     == Catch::Approx(0.4f));
    CHECK(cfg.shape.continentMaskResolution == 96);
}

TEST_CASE("Continent mask growth params round-trip through JSON", "[model][continent-mask][json]")
{
    planetgen::BodyConfig original;
    original.shape.continentCount          = 4;
    original.shape.continentSizeVariance   = 0.7f;
    original.shape.continentClustering     = 0.2f;
    original.shape.continentMaskResolution = 64;

    std::string json = planetgen::BodyConfigToJson(original);
    REQUIRE(!json.empty());

    planetgen::BodyConfig parsed;
    std::string err = planetgen::BodyConfigFromJson(json, parsed);
    REQUIRE(err.empty());

    CHECK(parsed.shape.continentCount          == 4);
    CHECK(parsed.shape.continentSizeVariance   == Catch::Approx(0.7f));
    CHECK(parsed.shape.continentClustering     == Catch::Approx(0.2f));
    CHECK(parsed.shape.continentMaskResolution == 64);
}

TEST_CASE("ContinentGrowthParams struct has correct size (std140 2-row)", "[model][continent-mask][param-block]")
{
    // The GLSL binding = 1 std140 block must match ContinentGrowthParams exactly.
    // Two 16-byte rows: uint[4] + float[4].
    CHECK(sizeof(planetgen::ContinentGrowthParams) == 32u);
    CHECK(alignof(planetgen::ContinentGrowthParams) == 16u);
}
