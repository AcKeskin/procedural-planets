// Shared lighting functions for all celestial body fragment shaders
// Blinn-Phong diffuse/specular, distance-adaptive fresnel, AO, atmospheric haze

#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

const vec3 FRESNEL_RIM_COLOR = vec3(0.4, 0.6, 1.0);
const float FRESNEL_RIM_INTENSITY = 0.3;
const float AO_MIN_FACTOR = 0.3;
const float HAZE_DISTANCE_RANGE = 4.0;

// Warm twilight tint shared by the surface sun (duskSunColor) and the atmosphere sky dusk, so the
// two redden to the same color at the terminator.
const vec3 DUSK_TINT = vec3(1.0, 0.55, 0.30);

// Blinn-Phong diffuse term
float calcDiffuse(vec3 normal, vec3 lightDir)
{
    return max(dot(normal, lightDir), 0.0);
}

// Soft day/night factor across the terminator. 1 = full day, 0 = full night, with a graded band
// around N·L = 0 instead of a hard clamp. Use the SPHERE normal (not the terrain normal) so the
// terminator follows the planet's curve, not local slope. `softness` is the half-width of the band.
float dayFactor(vec3 sphereNormal, vec3 lightDir, float softness)
{
    float ndl = dot(sphereNormal, lightDir);
    return smoothstep(-softness, softness, ndl);
}

// Warm the sunlight and redden it through the low-sun dusk band. Returns the effective sun color to
// multiply into the diffuse term: near-white at high sun, shifting toward warm/red as the sun nears
// the horizon (day factor drops toward 0), which is what sells the terminator on the surface.
vec3 duskSunColor(vec3 sunColor, float day)
{
    // dusk peaks in the twilight band (day ~0.5) and vanishes at full day/night
    float dusk = 1.0 - abs(day * 2.0 - 1.0);
    return mix(sunColor, sunColor * DUSK_TINT, dusk * 0.7);
}

// Cheap 3D hash → vec3 in [0,1] for jittering a light within its cell.
vec3 nightHash3(vec3 p)
{
    p = vec3(dot(p, vec3(127.1, 311.7, 74.7)),
             dot(p, vec3(269.5, 183.3, 246.1)),
             dot(p, vec3(113.5, 271.9, 124.6)));
    return fract(sin(p) * 43758.5453);
}

// Warm emissive city lights on the NIGHT side of habitable land. Fixes the failure modes of the
// earlier attempt: lights are POINTS (one jittered dot per grid cell with a sharp radial falloff,
// not a filled cell → pinpoints, not blocky patches), and a hard night gate (pow on 1-day) keeps
// them fully off daylit terrain. Coarse cells stay sparse so only some cells light up (settlements,
// not a uniform field). `habitability` (0..1, pre-masked to land) biases toward temperate,
// low-elevation, non-desert ground where settlements sit.
vec3 nightLights(vec3 worldDir, float day, float habitability, vec3 lightColor)
{
    // Hard night gate: fully zero unless the fragment is deep in night, so lights never leak onto
    // dim-but-daylit or twilight ground (only true dark side lights up). The tight [0,0.12] band
    // means anything the sun still touches at all stays dark.
    float night = 1.0 - smoothstep(0.0, 0.12, day);
    if (night < 0.002 || habitability < 0.002)
        return vec3(0.0);

    const float CELLS = 90.0;          // grid density over the unit sphere
    vec3 cell = floor(worldDir * CELLS);
    vec3 h = nightHash3(cell);

    // Only a fraction of cells are inhabited; h.z is the per-cell "is there a town here" roll.
    float inhabited = step(0.82, h.z);
    if (inhabited < 0.5)
        return vec3(0.0);

    // Distance from this fragment to the cell's jittered light point → sharp round dot.
    vec3 local = fract(worldDir * CELLS) - h; // h.xy jitters the point inside the cell
    float d = length(local.xy * 1.4);
    float dot_ = smoothstep(0.5, 0.0, d);     // 1 at the point, 0 at cell edge

    float intensity = dot_ * dot_ * habitability * night * 1.5;
    return lightColor * intensity;
}

// Blinn-Phong specular term
float calcSpecular(vec3 normal, vec3 lightDir, vec3 viewDir, float power, float strength)
{
    vec3 halfDir = normalize(lightDir + viewDir);
    return pow(max(dot(normal, halfDir), 0.0), power) * strength;
}

// Curvature-based ambient occlusion for spherical bodies
// Valleys (where terrain normal diverges from sphere normal) receive less ambient
float calcSphericalAO(vec3 terrainNormal, vec3 sphereNormal, float aoStrength)
{
    float normalAlignment = dot(terrainNormal, sphereNormal);
    return clamp(mix(1.0, normalAlignment, aoStrength), AO_MIN_FACTOR, 1.0);
}

// Combined diffuse + ambient + specular lighting
vec3 applyLighting(vec3 baseColor, vec3 normal, vec3 lightDir, vec3 viewDir,
                   float sunIntensity, float ambientLight, float aoFactor,
                   float specPower, float specStrength)
{
    float diff = calcDiffuse(normal, lightDir);
    float spec = calcSpecular(normal, lightDir, viewDir, specPower, specStrength);
    return baseColor * (ambientLight * aoFactor + diff * sunIntensity) + vec3(spec);
}

// Distance-adaptive fresnel rim
// Fades near surface, strong from orbit
vec3 calcFresnel(vec3 worldPos, vec3 cameraPos, vec3 sphereNormal,
                 float planetScale, float nearStrength, float farStrength, float fresnelPow)
{
    float camDist = length(cameraPos - worldPos);
    float camRadiiFromSurface = (camDist - planetScale) / planetScale;
    float fresnelT = smoothstep(0.0, 1.0, camRadiiFromSurface);
    float fresStrength = mix(nearStrength, farStrength, fresnelT);

    float fresnelDot = 1.0 + dot(normalize(worldPos - cameraPos), sphereNormal);
    float fresnel = clamp(fresStrength * pow(max(fresnelDot, 0.0), fresnelPow), 0.0, 1.0);

    return FRESNEL_RIM_COLOR * fresnel * FRESNEL_RIM_INTENSITY;
}

// Atmospheric perspective haze
// Distant terrain fades toward haze color
vec3 applyHaze(vec3 color, vec3 hazeColor, vec3 normal, vec3 lightDir,
               float camDist, float planetScale, float hazeStrength,
               float ambientLight, float sunIntensity)
{
    float camRadiiFromSurface = max((camDist - planetScale) / planetScale, 0.0);
    float hazeFactor = smoothstep(0.0, HAZE_DISTANCE_RANGE, camRadiiFromSurface) * hazeStrength;
    float hazeLighting = ambientLight + max(dot(normal, lightDir), 0.0) * sunIntensity * 0.5;
    return mix(color, hazeColor * hazeLighting, hazeFactor);
}

// Distance in planet-radii from camera to fragment
float calcDistanceRadii(vec3 worldPos, vec3 cameraPos, float planetScale)
{
    return length(cameraPos - worldPos) / planetScale;
}

#endif // LIGHTING_GLSL
