#version 450 core

in vec3 vNormal;
in vec3 vWorldPos;
in float vHeight;

out vec4 FragColor;

uniform vec3 uLightDir;
uniform vec3 uCameraPos;
uniform float uSeaLevel;

// Biome settings
uniform bool uUseBiomes;
uniform float uTempScale;
uniform float uHumidityScale;
uniform float uLatitudeInfluence;
uniform float uSnowLine;

// Biome colors
const vec3 SNOW = vec3(0.95, 0.95, 0.98);
const vec3 TUNDRA = vec3(0.6, 0.55, 0.45);
const vec3 TAIGA = vec3(0.2, 0.35, 0.25);
const vec3 FOREST = vec3(0.15, 0.4, 0.15);
const vec3 PLAINS = vec3(0.45, 0.55, 0.3);
const vec3 SAVANNA = vec3(0.7, 0.65, 0.4);
const vec3 DESERT = vec3(0.85, 0.75, 0.55);
const vec3 JUNGLE = vec3(0.1, 0.45, 0.15);
const vec3 BEACH = vec3(0.76, 0.70, 0.50);
const vec3 ROCK = vec3(0.40, 0.38, 0.35);

// Ocean colors (still used when below sea level)
const vec3 COLOR_DEEP_OCEAN = vec3(0.02, 0.05, 0.15);
const vec3 COLOR_SHALLOW_OCEAN = vec3(0.05, 0.15, 0.35);

// Simplex noise helpers
vec3 mod289(vec3 x) { return x - floor(x / 289.0) * 289.0; }
vec4 mod289(vec4 x) { return x - floor(x / 289.0) * 289.0; }
vec4 permute(vec4 x) { return mod289((x * 34.0 + 1.0) * x); }
vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }

float snoise(vec3 v)
{
    const vec2 C = vec2(1.0 / 6.0, 1.0 / 3.0);
    const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);

    vec3 i = floor(v + dot(v, C.yyy));
    vec3 x0 = v - i + dot(i, C.xxx);

    vec3 g = step(x0.yzx, x0.xyz);
    vec3 l = 1.0 - g;
    vec3 i1 = min(g.xyz, l.zxy);
    vec3 i2 = max(g.xyz, l.zxy);

    vec3 x1 = x0 - i1 + C.xxx;
    vec3 x2 = x0 - i2 + C.yyy;
    vec3 x3 = x0 - D.yyy;

    i = mod289(i);
    vec4 p = permute(permute(permute(
        i.z + vec4(0.0, i1.z, i2.z, 1.0))
        + i.y + vec4(0.0, i1.y, i2.y, 1.0))
        + i.x + vec4(0.0, i1.x, i2.x, 1.0));

    float n_ = 0.142857142857;
    vec3 ns = n_ * D.wyz - D.xzx;

    vec4 j = p - 49.0 * floor(p / 49.0);
    vec4 x_ = floor(j / 7.0);
    vec4 y_ = floor(j - 7.0 * x_);

    vec4 x = (x_ * 2.0 + 0.5) / 7.0 - 1.0;
    vec4 y = (y_ * 2.0 + 0.5) / 7.0 - 1.0;
    vec4 h = 1.0 - abs(x) - abs(y);

    vec4 b0 = vec4(x.xy, y.xy);
    vec4 b1 = vec4(x.zw, y.zw);

    vec4 s0 = floor(b0) * 2.0 + 1.0;
    vec4 s1 = floor(b1) * 2.0 + 1.0;
    vec4 sh = -step(h, vec4(0.0));

    vec4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
    vec4 a1 = b1.xzyw + s1.xzyw * sh.zzww;

    vec3 p0 = vec3(a0.xy, h.x);
    vec3 p1 = vec3(a0.zw, h.y);
    vec3 p2 = vec3(a1.xy, h.z);
    vec3 p3 = vec3(a1.zw, h.w);

    vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2,p2), dot(p3,p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;

    vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
    m = m * m;
    return 42.0 * dot(m * m, vec4(dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3)));
}

vec3 getBiomeColor(vec3 pos, float height, float seaLevel)
{
    // Beach zone
    float beachThreshold = seaLevel + 0.02;
    if (height < beachThreshold && height >= seaLevel)
    {
        return BEACH;
    }

    // High elevation = always snow/rock
    float normalizedHeight = (height - seaLevel) / (1.0 - seaLevel);
    if (normalizedHeight > uSnowLine)
    {
        return SNOW;
    }

    // Rock transition
    if (normalizedHeight > uSnowLine - 0.15)
    {
        float t = (normalizedHeight - (uSnowLine - 0.15)) / 0.15;
        return mix(ROCK, SNOW, t);
    }

    // Temperature: latitude-biased + noise
    vec3 unitPos = normalize(pos);
    float lat = abs(unitPos.y);  // 0 at equator, 1 at poles
    float tempNoise = snoise(pos * uTempScale) * 0.5 + 0.5;
    float temp = mix(tempNoise, 1.0 - lat, uLatitudeInfluence);

    // Humidity: pure noise with offset
    float humidity = snoise(pos * uHumidityScale + vec3(1000.0, 0.0, 0.0)) * 0.5 + 0.5;

    // 2D biome blend using temperature and humidity
    // Cold-dry to cold-wet
    vec3 coldDry = mix(SNOW, TUNDRA, smoothstep(0.0, 0.3, temp));
    vec3 coldWet = mix(TUNDRA, TAIGA, smoothstep(0.0, 0.3, temp));

    // Warm-dry to warm-wet
    vec3 warmDry = mix(PLAINS, DESERT, smoothstep(0.5, 1.0, temp));
    vec3 warmWet = mix(FOREST, JUNGLE, smoothstep(0.5, 1.0, temp));

    // Mid temperatures
    vec3 midDry = mix(TUNDRA, SAVANNA, smoothstep(0.3, 0.7, temp));
    vec3 midWet = mix(TAIGA, FOREST, smoothstep(0.3, 0.7, temp));

    // Blend based on temperature zones
    vec3 dryBiome, wetBiome;
    if (temp < 0.3)
    {
        dryBiome = coldDry;
        wetBiome = coldWet;
    }
    else if (temp < 0.7)
    {
        float t = (temp - 0.3) / 0.4;
        dryBiome = mix(coldDry, warmDry, t);
        wetBiome = mix(coldWet, warmWet, t);
    }
    else
    {
        dryBiome = warmDry;
        wetBiome = warmWet;
    }

    // Final blend based on humidity
    return mix(dryBiome, wetBiome, humidity);
}

vec3 GetTerrainColorLegacy(float height, float seaLevel)
{
    // Legacy height-based coloring
    float beachThreshold = seaLevel + 0.02;
    float grassThreshold = seaLevel + 0.15;
    float forestThreshold = seaLevel + 0.30;
    float rockThreshold = seaLevel + 0.50;
    float snowThreshold = seaLevel + 0.70;

    if (height < seaLevel - 0.1)
        return COLOR_DEEP_OCEAN;
    else if (height < seaLevel)
        return mix(COLOR_DEEP_OCEAN, COLOR_SHALLOW_OCEAN, (height - (seaLevel - 0.1)) / 0.1);
    else if (height < beachThreshold)
        return mix(COLOR_SHALLOW_OCEAN, BEACH, (height - seaLevel) / (beachThreshold - seaLevel));
    else if (height < grassThreshold)
        return mix(BEACH, PLAINS, (height - beachThreshold) / (grassThreshold - beachThreshold));
    else if (height < forestThreshold)
        return mix(PLAINS, FOREST, (height - grassThreshold) / (forestThreshold - grassThreshold));
    else if (height < rockThreshold)
        return mix(FOREST, ROCK, (height - forestThreshold) / (rockThreshold - forestThreshold));
    else if (height < snowThreshold)
        return mix(ROCK, SNOW, (height - rockThreshold) / (snowThreshold - rockThreshold));
    else
        return SNOW;
}

void main()
{
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(uLightDir);

    // Diffuse lighting
    float diff = max(dot(normal, lightDir), 0.0);

    // Ambient
    float ambient = 0.15;

    // Get terrain color
    vec3 terrainColor;
    if (vHeight < uSeaLevel)
    {
        // Underwater - show ocean floor colors
        if (vHeight < uSeaLevel - 0.1)
            terrainColor = COLOR_DEEP_OCEAN;
        else
            terrainColor = mix(COLOR_DEEP_OCEAN, COLOR_SHALLOW_OCEAN, (vHeight - (uSeaLevel - 0.1)) / 0.1);
    }
    else if (uUseBiomes)
    {
        terrainColor = getBiomeColor(vWorldPos, vHeight, uSeaLevel);
    }
    else
    {
        terrainColor = GetTerrainColorLegacy(vHeight, uSeaLevel);
    }

    // Final color
    vec3 color = terrainColor * (ambient + diff * 0.85);

    FragColor = vec4(color, 1.0);
}
