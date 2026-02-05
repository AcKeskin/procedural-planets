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

// Generate star brightness at a given direction
float starField(vec3 dir)
{
    // Use multiple scales for varied star sizes
    float stars = 0.0;

    // Small dim stars (many)
    vec3 starCoord1 = dir * 500.0;
    float star1 = hash(floor(starCoord1));
    float dist1 = length(fract(starCoord1) - 0.5);
    if (star1 > 0.995)
    {
        float brightness = smoothstep(0.5, 0.0, dist1) * (star1 - 0.995) * 200.0;
        stars += brightness * 0.3;
    }

    // Medium stars (fewer)
    vec3 starCoord2 = dir * 200.0;
    float star2 = hash(floor(starCoord2));
    float dist2 = length(fract(starCoord2) - 0.5);
    if (star2 > 0.992)
    {
        float brightness = smoothstep(0.5, 0.0, dist2) * (star2 - 0.992) * 125.0;
        stars += brightness * 0.6;
    }

    // Bright stars (rare)
    vec3 starCoord3 = dir * 80.0;
    float star3 = hash(floor(starCoord3));
    float dist3 = length(fract(starCoord3) - 0.5);
    if (star3 > 0.997)
    {
        float brightness = smoothstep(0.5, 0.0, dist3) * (star3 - 0.997) * 333.0;
        stars += brightness;
    }

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

    // Sun
    float sunAngle = acos(clamp(dot(viewDir, uLightDir), -1.0, 1.0));

    // Sun disc
    float sunDisc = smoothstep(uSunSize, uSunSize * 0.8, sunAngle);

    // Sun corona (glow around the sun)
    float coronaSize = uSunSize * 4.0;
    float corona = smoothstep(coronaSize, uSunSize, sunAngle);
    corona = pow(corona, 2.0);

    // Sun color with bloom
    vec3 sunContrib = uSunColor * sunDisc;
    sunContrib += uSunColor * corona * 0.3;

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
