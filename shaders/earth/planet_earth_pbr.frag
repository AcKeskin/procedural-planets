#version 450 core

// Realistic (PBR) planet surface — PROTOTYPE.
// Cook-Torrance lighting + procedural TRIPLANAR materials (rock / sand / grass / snow),
// no texture assets: albedo + roughness + detail-normal are generated in-shader from noise.
// Reuses the same vertex inputs and uniforms as planet_earth.frag, so it drops in by
// pointing earth.json shaderPaths.fragment here — no C++ change. Unused legacy uniforms
// (uShore*, uFlat*, ...) are simply not declared; SetRenderUniforms' extra sets no-op.

// lighting.glsl → calcFresnel, applyHaze, calcSphericalAO, calcDistanceRadii
#include "math.glsl"
#include "lighting.glsl"
#include "pbr.glsl"

in vec3 vNormal;
in vec3 vWorldPos;
in float vHeight;
in vec4 vShadingData; // climate: x=temp, y=moisture, z=detail, w=small

out vec4 FragColor;

uniform vec3  uLightDir;
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
uniform float uDetailFadeStart;
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

float valueNoise(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float n000 = hash13(i + vec3(0,0,0));
    float n100 = hash13(i + vec3(1,0,0));
    float n010 = hash13(i + vec3(0,1,0));
    float n110 = hash13(i + vec3(1,1,0));
    float n001 = hash13(i + vec3(0,0,1));
    float n101 = hash13(i + vec3(1,0,1));
    float n011 = hash13(i + vec3(0,1,1));
    float n111 = hash13(i + vec3(1,1,1));
    return mix(mix(mix(n000, n100, f.x), mix(n010, n110, f.x), f.y),
               mix(mix(n001, n101, f.x), mix(n011, n111, f.x), f.y), f.z);
}

float fbm3(vec3 p, int octaves)
{
    float sum = 0.0, amp = 0.5, freq = 1.0;
    for (int i = 0; i < octaves; ++i)
    {
        sum += amp * valueNoise(p * freq);
        freq *= 2.02;
        amp *= 0.5;
    }
    return sum;
}

// ---- triplanar sampling ----
// Blend weights from the geometric normal, sharpened so seams stay narrow.
vec3 triplanarWeights(vec3 n)
{
    vec3 w = pow(abs(n), vec3(4.0));
    return w / max(w.x + w.y + w.z, 1e-4);
}

// Scalar triplanar fbm of world position — isotropic surface grain with no UV stretch.
float triplanarFbm(vec3 wp, float scale, int octaves, vec3 w)
{
    float x = fbm3(wp.yzx * scale, octaves);
    float y = fbm3(wp.zxy * scale, octaves);
    float z = fbm3(wp.xyz * scale, octaves);
    return x * w.x + y * w.y + z * w.z;
}

// Material = (albedo, roughness). Procedural micro-variation baked into albedo.
struct Material { vec3 albedo; float roughness; };

Material rockMaterial(vec3 wp, vec3 w, float v)
{
    // grey granite with iron/ochre streaks
    vec3 base = mix(vec3(0.30, 0.29, 0.27), vec3(0.42, 0.40, 0.37), v);
    float ochre = smoothstep(0.55, 0.8, triplanarFbm(wp, 0.6, 4, w));
    base = mix(base, vec3(0.34, 0.26, 0.18), ochre * 0.5);
    return Material(base, mix(0.95, 0.7, v));
}

Material sandMaterial(vec3 wp, vec3 w, float v)
{
    vec3 base = mix(vec3(0.62, 0.52, 0.36), vec3(0.74, 0.66, 0.46), v);
    return Material(base, 0.9);
}

Material grassMaterial(vec3 wp, vec3 w, float v, float moisture)
{
    vec3 dry = vec3(0.34, 0.36, 0.18);
    vec3 wet = vec3(0.12, 0.30, 0.10);
    vec3 base = mix(dry, wet, clamp(moisture, 0.0, 1.0));
    base *= mix(0.8, 1.15, v); // patchy lightness
    return Material(base, 0.85);
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
    float distRadii = calcDistanceRadii(vWorldPos, uCameraPos, uPlanetScale);
    float detailFade = 1.0 - smoothstep(uDetailFadeStart, uDetailFadeStart * 2.0, distRadii);

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

    vec3 w = triplanarWeights(geoN);

    // Surface descriptors
    float steepness = 1.0 - clamp(dot(geoN, sphereNormal), 0.0, 1.0);
    float h = clamp((vHeight - uSeaLevel) / (uHeightMinMax.y - uSeaLevel), 0.0, 1.0);
    float temperature = vShadingData.x;
    float moisture    = vShadingData.y;

    // Procedural material variation fields (triplanar, world-space → seam-free, no stretch)
    float vFine   = triplanarFbm(vWorldPos, 3.0, 4, w);   // fine grain
    float vMedium = triplanarFbm(vWorldPos, 0.9, 4, w);   // patch-scale variation

    Material rock  = rockMaterial(vWorldPos, w, vFine);
    Material sand  = sandMaterial(vWorldPos, w, vFine);
    Material grass = grassMaterial(vWorldPos, w, vMedium, moisture);
    Material snow  = snowMaterial(vFine);

    // --- blend materials by slope / height / climate ---
    // flats: sand on dry-warm-low, grass otherwise
    float aridity = (1.0 - moisture) * smoothstep(0.35, 0.7, temperature);
    float sandW = smoothstep(0.35, 0.65, aridity) * (1.0 - smoothstep(0.12, 0.25, h));
    Material flatMat;
    flatMat.albedo    = mix(grass.albedo, sand.albedo, sandW);
    flatMat.roughness = mix(grass.roughness, sand.roughness, sandW);

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

    // --- procedural DETAIL NORMAL: perturb the geometric normal with a high-frequency noise
    // gradient so flat ground catches light as fine grain. CENTRAL difference (sample both sides
    // per axis) — a forward difference is asymmetric at noise extrema and spikes the normal into
    // specular RINGS (same forward-vs-central lesson as the vertex normals). Clamp the gradient so
    // an extremum can't throw the normal toward the light. Faded out at orbital distance. ---
    // eps is a SMALL fraction of a noise cell (cell size = 1/bumpScale in world space). Sampling a
    // full cell apart (the earlier bug) washed the gradient out; a tenth-cell step resolves it.
    float bumpScale = 45.0;
    float e = 0.1 / bumpScale;
    float nXp = triplanarFbm(vWorldPos + vec3(e, 0, 0), bumpScale, 3, w);
    float nXm = triplanarFbm(vWorldPos - vec3(e, 0, 0), bumpScale, 3, w);
    float nYp = triplanarFbm(vWorldPos + vec3(0, e, 0), bumpScale, 3, w);
    float nYm = triplanarFbm(vWorldPos - vec3(0, e, 0), bumpScale, 3, w);
    float nZp = triplanarFbm(vWorldPos + vec3(0, 0, e), bumpScale, 3, w);
    float nZm = triplanarFbm(vWorldPos - vec3(0, 0, e), bumpScale, 3, w);
    vec3 grad = vec3(nXp - nXm, nYp - nYm, nZp - nZm) / (2.0 * e);
    grad -= geoN * dot(grad, geoN);                  // keep perturbation tangential
    grad = clamp(grad, vec3(-8.0), vec3(8.0));        // cap extremum spikes (ring source) but allow grain
    float bumpStrength = mix(0.0, 0.025, detailFade) * (1.0 - snowW * 0.6);
    vec3 N = normalize(geoN - grad * bumpStrength);

    // --- Cook-Torrance shading ---
    // Ground (grass/soil/sand) is near-matte: real surfaces have almost no specular lobe, and a
    // tight lobe over the bumped normal is exactly what rings. Floor roughness high on non-rock so
    // the specular highlight stays broad and faint. Snow/wet rock keep a little more glint.
    float matte = max(rockW, snowW);
    float roughness = clamp(mat.roughness, mix(0.85, 0.4, matte), 1.0);
    float ao = calcSphericalAO(N, sphereNormal, uAOStrength);
    vec3 lit = cookTorrance(mat.albedo, 0.0, roughness,
                            N, viewDir, lightDir, vec3(uSunIntensity));
    vec3 ambient = mat.albedo * uAmbientLight * ao;
    vec3 color = lit + ambient;

    color += calcFresnel(vWorldPos, uCameraPos, sphereNormal,
                         uPlanetScale, uFresnelStrengthNear, uFresnelStrengthFar, uFresnelPow);
    color = applyHaze(color, uHazeColor, N, lightDir,
                      camDist, uPlanetScale, uHazeStrength, uAmbientLight, uSunIntensity);

    // gentle Reinhard tonemap so highlights roll off instead of clipping (no specular boost)
    color = color / (color + vec3(0.8));
    color = pow(color, vec3(0.9));   // mild lift

    FragColor = vec4(color, 1.0);
}
