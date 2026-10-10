// Reads the bloom mips (BloomPass) back at full resolution. Each mip is one ring of the glow,
// from tight (mip 1) to wide (mip 5).

uniform sampler2D uBloomTexture;
uniform float uBloomStrength;

const int BLOOM_FIRST_MIP = 1;
const int BLOOM_LAST_MIP = 5;

vec3 bloom(vec2 uv)
{
    vec3 sum = vec3(0.0);
    for (int mip = BLOOM_FIRST_MIP; mip <= BLOOM_LAST_MIP; ++mip)
    {
        // Four taps half a texel apart smooth the blocky bilinear upsample of the small mips.
        vec2 texel = 0.5 / vec2(textureSize(uBloomTexture, mip));
        sum += textureLod(uBloomTexture, uv + vec2(-texel.x, -texel.y), float(mip)).rgb
             + textureLod(uBloomTexture, uv + vec2( texel.x, -texel.y), float(mip)).rgb
             + textureLod(uBloomTexture, uv + vec2(-texel.x,  texel.y), float(mip)).rgb
             + textureLod(uBloomTexture, uv + vec2( texel.x,  texel.y), float(mip)).rgb;
    }
    return sum * (0.25 / float(BLOOM_LAST_MIP - BLOOM_FIRST_MIP + 1)) * uBloomStrength;
}
