// Noise functions for GPU compute shaders
// Port from CPU SimplexNoise with hash-based permutation

// Skewing factors for 3D simplex
const float F3 = 1.0 / 3.0;
const float G3 = 1.0 / 6.0;

// Gradient vectors for 3D simplex
const vec3 grad3[12] = vec3[12](
    vec3(1, 1, 0), vec3(-1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0),
    vec3(1, 0, 1), vec3(-1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, -1),
    vec3(0, 1, 1), vec3(0, -1, 1), vec3(0, 1, -1), vec3(0, -1, -1)
);

// Hash function for deterministic pseudo-random based on seed
uint hash(uint x, uint seed)
{
    x ^= seed;
    x = ((x >> 16) ^ x) * 0x45d9f3bu;
    x = ((x >> 16) ^ x) * 0x45d9f3bu;
    x = (x >> 16) ^ x;
    return x;
}

uint hash3(int x, int y, int z, uint seed)
{
    uint h = hash(uint(x) & 0xFFu, seed);
    h = hash(h ^ (uint(y) & 0xFFu), seed);
    h = hash(h ^ (uint(z) & 0xFFu), seed);
    return h;
}

int fastFloor(float x)
{
    int xi = int(x);
    return x < float(xi) ? xi - 1 : xi;
}

// 3D Simplex noise
float snoise(vec3 v, uint seed)
{
    float n0, n1, n2, n3;

    float s = (v.x + v.y + v.z) * F3;
    int i = fastFloor(v.x + s);
    int j = fastFloor(v.y + s);
    int k = fastFloor(v.z + s);

    float t = float(i + j + k) * G3;
    float X0 = float(i) - t;
    float Y0 = float(j) - t;
    float Z0 = float(k) - t;
    float x0 = v.x - X0;
    float y0 = v.y - Y0;
    float z0 = v.z - Z0;

    int i1, j1, k1;
    int i2, j2, k2;

    if (x0 >= y0)
    {
        if (y0 >= z0)
        {
            i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0;
        }
        else if (x0 >= z0)
        {
            i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1;
        }
        else
        {
            i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1;
        }
    }
    else
    {
        if (y0 < z0)
        {
            i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1;
        }
        else if (x0 < z0)
        {
            i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1;
        }
        else
        {
            i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0;
        }
    }

    float x1 = x0 - float(i1) + G3;
    float y1 = y0 - float(j1) + G3;
    float z1 = z0 - float(k1) + G3;
    float x2 = x0 - float(i2) + 2.0 * G3;
    float y2 = y0 - float(j2) + 2.0 * G3;
    float z2 = z0 - float(k2) + 2.0 * G3;
    float x3 = x0 - 1.0 + 3.0 * G3;
    float y3 = y0 - 1.0 + 3.0 * G3;
    float z3 = z0 - 1.0 + 3.0 * G3;

    // Get gradient indices
    int gi0 = int(hash3(i, j, k, seed) % 12u);
    int gi1 = int(hash3(i + i1, j + j1, k + k1, seed) % 12u);
    int gi2 = int(hash3(i + i2, j + j2, k + k2, seed) % 12u);
    int gi3 = int(hash3(i + 1, j + 1, k + 1, seed) % 12u);

    // Contribution from corners
    float t0 = 0.6 - x0 * x0 - y0 * y0 - z0 * z0;
    if (t0 < 0.0)
    {
        n0 = 0.0;
    }
    else
    {
        t0 *= t0;
        n0 = t0 * t0 * dot(grad3[gi0], vec3(x0, y0, z0));
    }

    float t1 = 0.6 - x1 * x1 - y1 * y1 - z1 * z1;
    if (t1 < 0.0)
    {
        n1 = 0.0;
    }
    else
    {
        t1 *= t1;
        n1 = t1 * t1 * dot(grad3[gi1], vec3(x1, y1, z1));
    }

    float t2 = 0.6 - x2 * x2 - y2 * y2 - z2 * z2;
    if (t2 < 0.0)
    {
        n2 = 0.0;
    }
    else
    {
        t2 *= t2;
        n2 = t2 * t2 * dot(grad3[gi2], vec3(x2, y2, z2));
    }

    float t3 = 0.6 - x3 * x3 - y3 * y3 - z3 * z3;
    if (t3 < 0.0)
    {
        n3 = 0.0;
    }
    else
    {
        t3 *= t3;
        n3 = t3 * t3 * dot(grad3[gi3], vec3(x3, y3, z3));
    }

    // Scale to [-1, 1] (original) then to [0, 1]
    return (32.0 * (n0 + n1 + n2 + n3) + 1.0) * 0.5;
}

// FBM (Fractal Brownian Motion)
float fbm(vec3 pos, vec3 offset, int numLayers, float scale, float persistence, float lacunarity, float multiplier, uint seed)
{
    float noiseSum = 0.0;
    float amplitude = 1.0;
    float frequency = scale;

    for (int i = 0; i < numLayers; i++)
    {
        // Remap snoise from [0,1] to [-1,1] for FBM summing
        float n = snoise(pos * frequency + offset, seed) * 2.0 - 1.0;
        noiseSum += n * amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return noiseSum * multiplier;
}

// Ridge noise for mountain-like features
float ridgeNoise(vec3 pos, vec3 offset, int numLayers, float scale, float persistence, float lacunarity, float multiplier, float power, float gain, uint seed)
{
    float noiseSum = 0.0;
    float amplitude = 1.0;
    float frequency = scale;
    float weight = 1.0;

    for (int i = 0; i < numLayers; i++)
    {
        float n = snoise(pos * frequency + offset, seed) * 2.0 - 1.0;
        n = 1.0 - abs(n);
        n = pow(n, power);
        n *= weight;
        weight = clamp(n * gain, 0.0, 1.0);

        noiseSum += n * amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return noiseSum * multiplier;
}

// Smoothed ridge noise (multi-sample averaging)
float smoothedRidgeNoise(vec3 pos, vec3 offset, int numLayers, float scale, float persistence, float lacunarity, float multiplier, float power, float gain, float smoothOffset, uint seed)
{
    vec3 normal = normalize(pos);
    vec3 axisA = cross(normal, vec3(0.0, 1.0, 0.0));
    if (length(axisA) < 0.001)
    {
        axisA = cross(normal, vec3(1.0, 0.0, 0.0));
    }
    axisA = normalize(axisA);
    vec3 axisB = cross(normal, axisA);

    float offsetDist = smoothOffset * 0.01;

    float s0 = ridgeNoise(pos, offset, numLayers, scale, persistence, lacunarity, multiplier, power, gain, seed);
    float s1 = ridgeNoise(pos - axisA * offsetDist, offset, numLayers, scale, persistence, lacunarity, multiplier, power, gain, seed);
    float s2 = ridgeNoise(pos + axisA * offsetDist, offset, numLayers, scale, persistence, lacunarity, multiplier, power, gain, seed);
    float s3 = ridgeNoise(pos - axisB * offsetDist, offset, numLayers, scale, persistence, lacunarity, multiplier, power, gain, seed);
    float s4 = ridgeNoise(pos + axisB * offsetDist, offset, numLayers, scale, persistence, lacunarity, multiplier, power, gain, seed);

    return (s0 + s1 + s2 + s3 + s4) / 5.0;
}

// Math utilities
float smoothMax(float a, float b, float k)
{
    return log(exp(k * a) + exp(k * b)) / k;
}

float smoothMin(float a, float b, float k)
{
    return -smoothMax(-a, -b, k);
}

float blend(float a, float b, float t)
{
    return a + (b - a) * clamp(t, 0.0, 1.0);
}
