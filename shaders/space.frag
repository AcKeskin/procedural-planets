#version 450 core

// Space background fragment shader
// Renders procedural starfield and sun

in vec2 vTexCoord;
in vec3 vViewRay;

out vec4 FragColor;

uniform vec3 uLightDir;      // Sun direction
uniform float uSunSize;      // Sun angular size (radians)
uniform vec3 uSunColor;      // Sun color
uniform float uStarDensity;  // Star density multiplier

// Hash function for procedural stars
float hash(vec3 p)
{
    p = fract(p * vec3(443.897, 441.423, 437.195));
    p += dot(p, p.yzx + 19.19);
    return fract((p.x + p.y) * p.z);
}

// 3D value noise
float noise(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    return mix(
        mix(mix(hash(i + vec3(0, 0, 0)), hash(i + vec3(1, 0, 0)), f.x),
            mix(hash(i + vec3(0, 1, 0)), hash(i + vec3(1, 1, 0)), f.x), f.y),
        mix(mix(hash(i + vec3(0, 0, 1)), hash(i + vec3(1, 0, 1)), f.x),
            mix(hash(i + vec3(0, 1, 1)), hash(i + vec3(1, 1, 1)), f.x), f.y),
        f.z);
}

// One star layer: a stable star per grid cell in direction-space (no swim under camera translate),
// with a smooth radial dot (no hard threshold → no twinkle-alias under rotation). `cut` is the
// sparsity (higher = fewer stars), `weight` the layer brightness.
float starLayer(vec3 dir, float scale, float cut, float weight)
{
    vec3 coord = dir * scale;
    vec3 cell = floor(coord);
    float rnd = hash(cell);
    if (rnd < cut)
        return 0.0;
    float presence = (rnd - cut) / (1.0 - cut); // 0..1 how bright this cell's star is
    float dist = length(fract(coord) - 0.5);
    float dot_ = smoothstep(0.5, 0.0, dist);
    return dot_ * dot_ * presence * weight;
}

float starField(vec3 dir)
{
    float stars = starLayer(dir, 500.0, 0.995, 0.3)   // small dim, many
                + starLayer(dir, 200.0, 0.992, 0.6)   // medium
                + starLayer(dir, 80.0, 0.997, 1.0);   // bright, rare
    return stars * uStarDensity;
}

// Star color based on temperature (simplified)
vec3 starColor(vec3 dir)
{
    float temp = hash(dir * 100.0 + vec3(1000.0));

    // Blue-white to yellow-orange gradient
    vec3 coolStar = vec3(0.8, 0.85, 1.0);   // Blue-white
    vec3 warmStar = vec3(1.0, 0.9, 0.7);    // Yellow
    vec3 redStar = vec3(1.0, 0.6, 0.4);     // Orange-red

    if (temp < 0.5)
        return mix(redStar, warmStar, temp * 2.0);
    else
        return mix(warmStar, coolStar, (temp - 0.5) * 2.0);
}

void main()
{
    vec3 viewDir = normalize(vViewRay);

    // Deep space background color (very dark blue-black)
    vec3 spaceColor = vec3(0.0, 0.0, 0.02);

    // Add subtle nebula-like color variation
    float nebulaPattern = noise(viewDir * 3.0) * 0.5 + noise(viewDir * 7.0) * 0.25;
    vec3 nebulaColor = mix(
        vec3(0.02, 0.0, 0.04),  // Purple tint
        vec3(0.0, 0.02, 0.04),  // Cyan tint
        nebulaPattern
    );
    spaceColor += nebulaColor * 0.3;

    // Stars
    float starBrightness = starField(viewDir);
    vec3 stars = starColor(viewDir) * starBrightness;

    // Sun. uLightDir points FROM the sun, so the disc sits in the -uLightDir direction (matching
    // the lit side of the planet).
    vec3 sunDir = -normalize(uLightDir);
    float sunAngle = acos(clamp(dot(viewDir, sunDir), -1.0, 1.0));

    // Bright disc core.
    float sunDisc = smoothstep(uSunSize, uSunSize * 0.8, sunAngle);

    // Layered glare: a tight bright inner bloom + a wide soft halo, both falling off radially from
    // the disc. Gives the sun a photographic glare instead of a flat disc.
    float inner = smoothstep(uSunSize * 6.0, uSunSize, sunAngle);
    inner = pow(inner, 2.5);
    float halo = smoothstep(uSunSize * 24.0, uSunSize, sunAngle);
    halo = pow(halo, 3.0);

    vec3 sunContrib = uSunColor * (sunDisc * 3.0 + inner * 0.9 + halo * 0.25);

    // Combine everything
    vec3 finalColor = spaceColor + stars + sunContrib;

    // Subtle HDR-like glow for bright areas
    float luminance = dot(finalColor, vec3(0.299, 0.587, 0.114));
    if (luminance > 1.0)
    {
        finalColor = finalColor / luminance * (1.0 + (luminance - 1.0) * 0.5);
    }

    FragColor = vec4(finalColor, 1.0);
}
