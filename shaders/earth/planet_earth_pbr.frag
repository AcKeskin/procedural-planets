#version 450 core

// Realistic (PBR) planet surface — PROTOTYPE.
// Cook-Torrance lighting + procedural materials (rock / sand / grass / snow),
// no texture assets: albedo + roughness + detail-normal are generated in-shader from noise.
// Reuses the same vertex inputs and uniforms as planet_earth.frag, so it drops in by
// pointing earth.json shaderPaths.fragment here — no C++ change. Unused legacy uniforms
// (uShore*, uFlat*, ...) are simply not declared; SetRenderUniforms' extra sets no-op.

// lighting.glsl → calcFresnel, applyHaze, calcSphericalAO, calcDistanceRadii
// clouds.glsl → cloudCoverage: soft shadow that matches the cloud shell
#include "math.glsl"
#include "lighting.glsl"
#include "clouds.glsl"
#include "pbr.glsl"
#include "ground_material.glsl"
#include "biome_palette.glsl"

in vec3 vNormal;
in vec3 vWorldPos;
in float vHeight;
in vec4 vShadingData; // climate: x=temp, y=moisture, z=detail, w=small

out vec4 FragColor;

uniform vec3  uLightDir;
uniform vec3  uSunColor;          // warms the key light; reddens through the dusk band
uniform float uTerminatorSoftness; // half-width of the soft day/night band (0 = hard edge)
uniform float uNightLightStrength; // emissive city lights on the night side (0 = off)

// Cloud shadow: sample the shared coverage field along the sun so shadows track the visible clouds.
uniform float uCloudScale;
uniform float uCloudCoverage;
uniform float uCloudSpeed;
uniform float uCloudTime;
uniform float uCloudShadowStrength;
uniform float uCloudAltitude;

uniform vec3  uCameraPos;
uniform float uSeaLevel;
uniform vec2  uHeightMinMax;
uniform float uSnowLine;

uniform float uSunIntensity;
uniform float uAmbientLight;

// Atmosphere integration (shared with the stylized path)
uniform float uPlanetScale;
uniform float uFresnelStrengthNear;
uniform float uFresnelStrengthFar;
uniform float uFresnelPow;
uniform float uHazeStrength;
uniform vec3  uHazeColor;
uniform float uAOStrength;

const vec3 DEEP_OCEAN    = vec3(0.015, 0.05, 0.13);
const vec3 SHALLOW_OCEAN = vec3(0.04, 0.16, 0.34);
const vec3 COASTAL       = vec3(0.10, 0.38, 0.40);

// ---- procedural noise (self-contained; no dependency on the compute-side noise) ----
float hash13(vec3 p)
{
    p = fract(p * 0.1031);
    p += dot(p, p.zyx + 31.32);
    return fract((p.x + p.y) * p.z);
}

// Value noise and its analytic gradient: .x = value in [0,1], .yzw = d/dp. One evaluation gives
// both the albedo grain and the bump, where a finite difference needs six.
vec4 valueNoiseD(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    vec3 u = f * f * (3.0 - 2.0 * f);
    vec3 du = 6.0 * f * (1.0 - f);

    float a = hash13(i + vec3(0,0,0));
    float b = hash13(i + vec3(1,0,0));
    float c = hash13(i + vec3(0,1,0));
    float d = hash13(i + vec3(1,1,0));
    float e = hash13(i + vec3(0,0,1));
    float f1 = hash13(i + vec3(1,0,1));
    float g = hash13(i + vec3(0,1,1));
    float h = hash13(i + vec3(1,1,1));

    float k0 = a, k1 = b - a, k2 = c - a, k3 = e - a;
    float k4 = a - b - c + d, k5 = a - c - e + g, k6 = a - b - e + f1;
    float k7 = -a + b + c - d + e - f1 - g + h;

    float value = k0 + k1 * u.x + k2 * u.y + k3 * u.z + k4 * u.x * u.y + k5 * u.y * u.z
                + k6 * u.z * u.x + k7 * u.x * u.y * u.z;
    vec3 grad = du * vec3(k1 + k4 * u.y + k6 * u.z + k7 * u.y * u.z,
                          k2 + k5 * u.z + k4 * u.x + k7 * u.z * u.x,
                          k3 + k6 * u.x + k5 * u.y + k7 * u.x * u.y);
    return vec4(value, grad);
}

// Weight of a noise octave of this frequency at this pixel footprint (both in matPos units). An
// octave fades out between 6.7 and 2.9 pixels per cycle: finer than that it only shimmers.
float octaveWeight(float frequency, float footprint)
{
    return 1.0 - smoothstep(0.15, 0.35, frequency * footprint);
}

// fbm that adds octaves as the footprint shrinks, so close views gain grain and far views never
// alias. A dropped octave contributes its mean (0.5), so brightness does not drift with distance.
// Returns .x = value, .yzw = gradient.
vec4 fbmFootprint(vec3 p, float baseFrequency, int maxOctaves, float footprint, float gain)
{
    vec4 sum = vec4(0.0);
    float amp = 0.5;
    float frequency = baseFrequency;
    for (int i = 0; i < maxOctaves; ++i)
    {
        float w = octaveWeight(frequency, footprint);
        if (w <= 0.0)
        {
            sum.x += amp * 0.5 * (1.0 - pow(gain, float(maxOctaves - i))) / (1.0 - gain);
            break;
        }
        vec4 n = valueNoiseD(p * frequency);
        sum.x += amp * mix(0.5, n.x, w);
        sum.yzw += amp * w * frequency * n.yzw;
        frequency *= 2.02;
        amp *= gain;
    }
    return sum;
}

// Planet radius the material and detail-normal noise frequencies were tuned at.
const float kMaterialReferenceRadius = 10.0;

// Material = (albedo, roughness). Procedural micro-variation baked into albedo.
struct Material { vec3 albedo; float roughness; };

Material rockMaterial(vec3 wp, float footprint, float v)
{
    // grey granite with iron/ochre streaks
    vec3 base = mix(vec3(0.30, 0.29, 0.27), vec3(0.42, 0.40, 0.37), v);
    float ochre = smoothstep(0.55, 0.8, fbmFootprint(wp, 0.6, 4, footprint, 0.5).x);
    base = mix(base, vec3(0.34, 0.26, 0.18), ochre * 0.5);
    return Material(base, mix(0.95, 0.7, v));
}

Material sandMaterial(float v)
{
    vec3 base = mix(vec3(0.62, 0.52, 0.36), vec3(0.74, 0.66, 0.46), v);
    return Material(base, 0.9);
}

// Flat ground: the palette biome colour with patchy lightness.
Material flatGroundMaterial(vec3 biomeColor, float v)
{
    return Material(biomeColor * mix(0.8, 1.15, v), 0.85);
}

Material snowMaterial(float v)
{
    return Material(mix(vec3(0.86, 0.88, 0.93), vec3(0.97, 0.98, 1.0), v), 0.4);
}

void main()
{
    vec3 sphereNormal = normalize(vWorldPos);
    vec3 geoN = length(vNormal) > 1e-4 ? normalize(vNormal) : sphereNormal;
    if (dot(geoN, sphereNormal) < 0.0) geoN = -geoN;

    vec3 lightDir = -normalize(uLightDir);          // toward the sun
    vec3 viewDir  = normalize(uCameraPos - vWorldPos);
    float camDist = length(uCameraPos - vWorldPos);

    // ---- ocean: simple PBR water, fresnel-tinted ----
    if (vHeight < uSeaLevel)
    {
        float depth = clamp((uSeaLevel - vHeight) / 0.08, 0.0, 1.0);
        vec3 waterAlbedo = mix(COASTAL, DEEP_OCEAN, depth);
        vec3 lit = cookTorrance(waterAlbedo, 0.0, 0.08, sphereNormal, viewDir,
                                lightDir, vec3(uSunIntensity));
        vec3 color = lit + waterAlbedo * uAmbientLight;
        color += calcFresnel(vWorldPos, uCameraPos, sphereNormal,
                             uPlanetScale, uFresnelStrengthNear, uFresnelStrengthFar, uFresnelPow);
        color = applyHaze(color, uHazeColor, sphereNormal, lightDir,
                          camDist, uPlanetScale, uHazeStrength, uAmbientLight, uSunIntensity);
        FragColor = vec4(color, 1.0);
        return;
    }

    // Surface descriptors
    float steepness = 1.0 - clamp(dot(geoN, sphereNormal), 0.0, 1.0);
    float h = clamp((vHeight - uSeaLevel) / (uHeightMinMax.y - uSeaLevel), 0.0, 1.0);
    float temperature = vShadingData.x;
    float moisture    = vShadingData.y;

    // Material noise runs on the position scaled to the radius it was tuned at, so the grain
    // keeps its size relative to the planet at any radius. In raw world units a radius-5000
    // planet gets 500x finer grain that aliases to per-pixel noise, and the large coordinates
    // cost the noise input its precision. At the reference radius the scale is exactly 1.
    vec3 matPos = vWorldPos * (kMaterialReferenceRadius / uPlanetScale);

    // matPos units per pixel: decides which noise octaves this pixel can show.
    float footprint = max(length(dFdx(matPos)), length(dFdy(matPos)));

    // Material variation fields (3D noise of position: seam-free and unstretched by construction).
    // The fine grain gains octaves down to a few pixels as the camera closes in.
    vec4 fine = fbmFootprint(matPos, 3.0, 12, footprint, 0.5);
    float vFine   = fine.x;
    float vMedium = fbmFootprint(matPos, 0.9, 4, footprint, 0.5).x; // patch-scale variation

    Biome biome = whittakerBiome(temperature, moisture);

    Material rock  = rockMaterial(matPos, footprint, vFine);
    Material sand  = sandMaterial(vFine);
    Material snow  = snowMaterial(vFine);

    // --- blend materials by slope / height / climate ---
    // Flats: the biome colour; beaches (dry, warm, low) blend to sand.
    float aridity = (1.0 - moisture) * smoothstep(0.35, 0.7, temperature);
    float sandW = smoothstep(0.35, 0.65, aridity) * (1.0 - smoothstep(0.12, 0.25, h));
    Material flatMat = flatGroundMaterial(biome.color, vMedium);
    flatMat.albedo    = mix(flatMat.albedo, sand.albedo, sandW);
    flatMat.roughness = mix(flatMat.roughness, sand.roughness, sandW);

    // rock takes over on slope (with noisy threshold so the line isn't clean)
    float rockThresh = 0.35 + (vMedium - 0.5) * 0.25;
    float rockW = smoothstep(rockThresh - 0.12, rockThresh + 0.12, steepness);

    Material mat;
    mat.albedo    = mix(flatMat.albedo, rock.albedo, rockW);
    mat.roughness = mix(flatMat.roughness, rock.roughness, rockW);

    // snow: altitude-driven, climate lowers the line; sheds on steep rock
    float snowLineEff = uSnowLine - (1.0 - temperature) * 0.04;
    float snowW = smoothstep(snowLineEff - 0.20, snowLineEff + 0.05, h);
    snowW = max(snowW, smoothstep(0.04, 0.0, temperature) * 0.5);   // polar cap
    snowW *= 1.0 - smoothstep(0.55, 0.85, steepness) * 0.7;
    mat.albedo    = mix(mat.albedo, snow.albedo, snowW);
    mat.roughness = mix(mat.roughness, snow.roughness, snowW);

    // Fine grain on every material (grass took none before), weaker on snow.
    mat.albedo *= mix(1.0, mix(0.72, 1.28, vFine), 1.0 - snowW * 0.7);

    // Texture detail: the same blend as above, spread over the five texture layers (dry flats get
    // the soil layer). Only layers with real weight are sampled.
    float flatW = (1.0 - rockW) * (1.0 - snowW);
    float desert = smoothstep(0.45, 0.75, aridity);
    sandW = max(sandW, desert);
    float soilShare = max(smoothstep(0.6, 0.85, 1.0 - moisture) * 0.6, biome.polar * 0.7);
    float layerW[5];
    layerW[GROUND_ROCK]  = rockW * (1.0 - snowW);
    layerW[GROUND_SNOW]  = snowW;
    layerW[GROUND_SAND]  = flatW * sandW;
    layerW[GROUND_SOIL]  = flatW * (1.0 - sandW) * soilShare;
    layerW[GROUND_GRASS] = flatW * (1.0 - sandW) * (1.0 - soilShare);

    vec3 tw = groundTriplanarWeights(geoN);
    vec3 texAlbedo = vec3(0.0);
    vec3 texNormal = vec3(0.0);
    float texTotal = 0.0;
    for (int layer = 0; layer < 5; ++layer)
    {
        if (layerW[layer] < 0.03)
            continue;
        GroundSample g = groundLayer(matPos, tw, layer, footprint);
        texAlbedo += g.albedo * layerW[layer];
        texNormal += g.normal * layerW[layer];
        texTotal += layerW[layer];
    }
    if (texTotal > 0.0)
    {
        mat.albedo *= texAlbedo / texTotal;
        texNormal /= texTotal;
    }

    // --- DETAIL NORMAL from the analytic gradient of the bump fbm. Octaves join as the footprint
    // shrinks, so the grain sharpens toward the ground and never aliases from far away. Each octave
    // tilts the normal about the same amount (amp halves, frequency doubles). ---
    const float kBumpFrequency = 45.0;
    const float kBumpTilt = 0.012; // normal tilt per unit of gradient
    vec3 grad = fbmFootprint(matPos, kBumpFrequency, 8, footprint, 0.5).yzw;
    grad -= geoN * dot(grad, geoN); // keep the perturbation tangential
    float bumpStrength = kBumpTilt * (1.0 - snowW * 0.6);
    const float kTextureBump = 0.6; // tilt from the material normal maps
    vec3 N = normalize(geoN - grad * bumpStrength + (texNormal - geoN * dot(texNormal, geoN)) * kTextureBump);

    // --- Cook-Torrance shading ---
    // Ground (grass/soil/sand) is near-matte: real surfaces have almost no specular lobe, and a
    // tight lobe over the bumped normal is exactly what rings. Floor roughness high on non-rock so
    // the specular highlight stays broad and faint. Snow/wet rock keep a little more glint.
    float matte = max(rockW, snowW);
    float roughness = clamp(mat.roughness, mix(0.85, 0.4, matte), 1.0);
    float ao = calcSphericalAO(N, sphereNormal, uAOStrength);

    // Celestial lighting: warm/dusk-reddened key light. Cook-Torrance's own max(N·L,0) already
    // falls the direct light off across the terminator, so we DON'T re-multiply by a day factor
    // (that double-darkened low-sun-angle daylit regions). The `day` value is used only for the
    // dusk color shift and for gating night lights — not for attenuating the diffuse.
    float day = dayFactor(sphereNormal, lightDir, uTerminatorSoftness);
    vec3 sunRadiance = duskSunColor(uSunColor, day) * uSunIntensity;

    // Soft cloud shadow: sample the shared coverage field where this fragment's sun ray crosses the
    // cloud shell (surface direction pushed toward the sun), and dim the direct light. Only on the
    // lit side (× day), gated by uCloudShadowStrength (0 when clouds are off).
    vec3 shadowDir = normalize(sphereNormal + lightDir * uCloudAltitude * 6.0);
    float cloudShadow = cloudCoverage(shadowDir, uCloudScale, uCloudCoverage, uCloudSpeed, uCloudTime);
    sunRadiance *= 1.0 - cloudShadow * uCloudShadowStrength * day;

    vec3 lit = cookTorrance(mat.albedo, 0.0, roughness, N, viewDir, lightDir, sunRadiance);
    vec3 ambient = mat.albedo * uAmbientLight * ao;
    vec3 color = lit + ambient;

    color += calcFresnel(vWorldPos, uCameraPos, sphereNormal,
                         uPlanetScale, uFresnelStrengthNear, uFresnelStrengthFar, uFresnelPow);
    color = applyHaze(color, uHazeColor, N, lightDir,
                      camDist, uPlanetScale, uHazeStrength, uAmbientLight, uSunIntensity);

    // Night-side city lights on habitable land: temperate, moist-enough, low-elevation ground
    // (where settlements sit), never snow/desert/mountain. Land-only here by construction (ocean
    // returned early). Warm sodium-lamp color, hard-gated to the night side inside nightLights().
    float habitability = uNightLightStrength
                       * smoothstep(0.35, 0.6, temperature) * (1.0 - smoothstep(0.75, 0.95, temperature))
                       * smoothstep(0.2, 0.5, moisture)
                       * (1.0 - smoothstep(0.3, 0.6, h))   // low elevation
                       * (1.0 - snowW);
    color += nightLights(normalize(vWorldPos), day, habitability, vec3(1.0, 0.8, 0.45));

    // No roll-off here: the scene is HDR and tonemap.glsl rolls highlights off once, at the end.

    FragColor = vec4(color, 1.0);
}
