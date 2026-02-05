// ============================================================================
// noise.glsl - Procedural noise functions for terrain generation
// ============================================================================

#ifndef NOISE_GLSL
#define NOISE_GLSL

// Simplex noise constants
const float F3 = 1.0 / 3.0;
const float G3 = 1.0 / 6.0;

// Gradient vectors for 3D simplex noise
const vec3 grad3[12] = vec3[12](
    vec3(1, 1, 0), vec3(-1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0),
    vec3(1, 0, 1), vec3(-1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, -1),
    vec3(0, 1, 1), vec3(0, -1, 1), vec3(0, 1, -1), vec3(0, -1, -1)
);

// ============================================================================
// Hash Functions
// ============================================================================

uint noiseHash(uint x, uint seed)
{
    x ^= seed;
    x = ((x >> 16) ^ x) * 0x45d9f3bu;
    x = ((x >> 16) ^ x) * 0x45d9f3bu;
    x = (x >> 16) ^ x;
    return x;
}

uint noiseHash3(int x, int y, int z, uint seed)
{
    uint h = noiseHash(uint(x) & 0xFFu, seed);
    h = noiseHash(h ^ (uint(y) & 0xFFu), seed);
    h = noiseHash(h ^ (uint(z) & 0xFFu), seed);
    return h;
}

int noiseFastFloor(float x)
{
    int xi = int(x);
    return x < float(xi) ? xi - 1 : xi;
}

// ============================================================================
// Simplex 3D Noise
// Returns value in range [0, 1]
// ============================================================================

float snoise(vec3 v, uint seed)
{
    float n0, n1, n2, n3;

    // Skew input space to determine simplex cell
    float s = (v.x + v.y + v.z) * F3;
    int i = noiseFastFloor(v.x + s);
    int j = noiseFastFloor(v.y + s);
    int k = noiseFastFloor(v.z + s);

    // Unskew cell origin back to (x,y,z) space
    float t = float(i + j + k) * G3;
    float X0 = float(i) - t;
    float Y0 = float(j) - t;
    float Z0 = float(k) - t;
    float x0 = v.x - X0;
    float y0 = v.y - Y0;
    float z0 = v.z - Z0;

    // Determine which simplex we are in
    int i1, j1, k1, i2, j2, k2;

    if (x0 >= y0)
    {
        if (y0 >= z0) { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0; }
        else if (x0 >= z0) { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1; }
        else { i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1; }
    }
    else
    {
        if (y0 < z0) { i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1; }
        else if (x0 < z0) { i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1; }
        else { i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0; }
    }

    // Offsets for corners
    float x1 = x0 - float(i1) + G3;
    float y1 = y0 - float(j1) + G3;
    float z1 = z0 - float(k1) + G3;
    float x2 = x0 - float(i2) + 2.0 * G3;
    float y2 = y0 - float(j2) + 2.0 * G3;
    float z2 = z0 - float(k2) + 2.0 * G3;
    float x3 = x0 - 1.0 + 3.0 * G3;
    float y3 = y0 - 1.0 + 3.0 * G3;
    float z3 = z0 - 1.0 + 3.0 * G3;

    // Gradient indices
    int gi0 = int(noiseHash3(i, j, k, seed) % 12u);
    int gi1 = int(noiseHash3(i + i1, j + j1, k + k1, seed) % 12u);
    int gi2 = int(noiseHash3(i + i2, j + j2, k + k2, seed) % 12u);
    int gi3 = int(noiseHash3(i + 1, j + 1, k + 1, seed) % 12u);

    // Corner contributions
    float t0 = 0.6 - x0 * x0 - y0 * y0 - z0 * z0;
    n0 = (t0 < 0.0) ? 0.0 : t0 * t0 * t0 * t0 * dot(grad3[gi0], vec3(x0, y0, z0));

    float t1 = 0.6 - x1 * x1 - y1 * y1 - z1 * z1;
    n1 = (t1 < 0.0) ? 0.0 : t1 * t1 * t1 * t1 * dot(grad3[gi1], vec3(x1, y1, z1));

    float t2 = 0.6 - x2 * x2 - y2 * y2 - z2 * z2;
    n2 = (t2 < 0.0) ? 0.0 : t2 * t2 * t2 * t2 * dot(grad3[gi2], vec3(x2, y2, z2));

    float t3 = 0.6 - x3 * x3 - y3 * y3 - z3 * z3;
    n3 = (t3 < 0.0) ? 0.0 : t3 * t3 * t3 * t3 * dot(grad3[gi3], vec3(x3, y3, z3));

    // Scale to [0, 1]
    return (32.0 * (n0 + n1 + n2 + n3) + 1.0) * 0.5;
}

// ============================================================================
// Fractal Brownian Motion
// Accumulates noise over octaves with decreasing amplitude
// ============================================================================

float fbm(vec3 pos, int octaves, float lacunarity, float persistence)
{
    float sum = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;
    float maxValue = 0.0;

    for (int i = 0; i < octaves; i++)
    {
        sum += snoise(pos * frequency, uint(i)) * amplitude;
        maxValue += amplitude;
        frequency *= lacunarity;
        amplitude *= persistence;
    }

    // Normalize to [-1, 1]
    return (sum / maxValue) * 2.0 - 1.0;
}

// Seeded variant with offset
float fbmSeeded(vec3 pos, vec3 offset, int octaves, float scale, float lacunarity, float persistence, uint seed)
{
    float sum = 0.0;
    float amplitude = 1.0;
    float frequency = scale;

    for (int i = 0; i < octaves; i++)
    {
        float n = snoise(pos * frequency + offset, seed) * 2.0 - 1.0;
        sum += n * amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return sum;
}

// ============================================================================
// Ridge Noise
// Uses 1 - abs(noise) for sharp ridges
// power controls sharpness, gain controls weight propagation
// ============================================================================

float ridgeNoise(vec3 pos, int octaves, float lacunarity, float persistence, float power, float gain)
{
    float sum = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;
    float weight = 1.0;
    float maxValue = 0.0;

    for (int i = 0; i < octaves; i++)
    {
        float n = snoise(pos * frequency, uint(i)) * 2.0 - 1.0;
        n = 1.0 - abs(n);
        n = pow(n, power);
        n *= weight;
        weight = clamp(n * gain, 0.0, 1.0);

        sum += n * amplitude;
        maxValue += amplitude;
        frequency *= lacunarity;
        amplitude *= persistence;
    }

    return sum / maxValue;
}

// Seeded variant with offset and scale
float ridgeNoiseSeeded(vec3 pos, vec3 offset, int octaves, float scale, float lacunarity, float persistence, float power, float gain, uint seed)
{
    float sum = 0.0;
    float amplitude = 1.0;
    float frequency = scale;
    float weight = 1.0;

    for (int i = 0; i < octaves; i++)
    {
        float n = snoise(pos * frequency + offset, seed) * 2.0 - 1.0;
        n = 1.0 - abs(n);
        n = pow(n, power);
        n *= weight;
        weight = clamp(n * gain, 0.0, 1.0);

        sum += n * amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return sum;
}

// ============================================================================
// Smoothed Ridge Noise
// Samples at multiple nearby points to reduce harsh peaks
// ============================================================================

float smoothedRidgeNoise(vec3 pos, int octaves, float lacunarity, float persistence, float power, float gain, float smoothing)
{
    vec3 normal = normalize(pos);

    // Compute tangent basis
    vec3 axisA = cross(normal, vec3(0.0, 1.0, 0.0));
    if (length(axisA) < 0.001)
        axisA = cross(normal, vec3(1.0, 0.0, 0.0));
    axisA = normalize(axisA);
    vec3 axisB = cross(normal, axisA);

    float offsetDist = smoothing * 0.01;

    // Sample at center and 4 surrounding points
    float s0 = ridgeNoise(pos, octaves, lacunarity, persistence, power, gain);
    float s1 = ridgeNoise(pos - axisA * offsetDist, octaves, lacunarity, persistence, power, gain);
    float s2 = ridgeNoise(pos + axisA * offsetDist, octaves, lacunarity, persistence, power, gain);
    float s3 = ridgeNoise(pos - axisB * offsetDist, octaves, lacunarity, persistence, power, gain);
    float s4 = ridgeNoise(pos + axisB * offsetDist, octaves, lacunarity, persistence, power, gain);

    return (s0 + s1 + s2 + s3 + s4) * 0.2;
}

// Seeded variant with offset and scale
float smoothedRidgeNoiseSeeded(vec3 pos, vec3 offset, int octaves, float scale, float lacunarity, float persistence, float power, float gain, float smoothing, uint seed)
{
    vec3 normal = normalize(pos);

    vec3 axisA = cross(normal, vec3(0.0, 1.0, 0.0));
    if (length(axisA) < 0.001)
        axisA = cross(normal, vec3(1.0, 0.0, 0.0));
    axisA = normalize(axisA);
    vec3 axisB = cross(normal, axisA);

    float offsetDist = smoothing * 0.01;

    float s0 = ridgeNoiseSeeded(pos, offset, octaves, scale, lacunarity, persistence, power, gain, seed);
    float s1 = ridgeNoiseSeeded(pos - axisA * offsetDist, offset, octaves, scale, lacunarity, persistence, power, gain, seed);
    float s2 = ridgeNoiseSeeded(pos + axisA * offsetDist, offset, octaves, scale, lacunarity, persistence, power, gain, seed);
    float s3 = ridgeNoiseSeeded(pos - axisB * offsetDist, offset, octaves, scale, lacunarity, persistence, power, gain, seed);
    float s4 = ridgeNoiseSeeded(pos + axisB * offsetDist, offset, octaves, scale, lacunarity, persistence, power, gain, seed);

    return (s0 + s1 + s2 + s3 + s4) * 0.2;
}

#endif // NOISE_GLSL
