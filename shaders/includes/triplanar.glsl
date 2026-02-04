// Triplanar texture mapping utilities for spherical surfaces
// Reference: medium.com/@bgolus/normal-mapping-for-a-triplanar-shader-10bf39dca05a

#ifndef TRIPLANAR_GLSL
#define TRIPLANAR_GLSL

// Calculate blend weights from surface normal
// Squaring makes all values positive and increases blend sharpness
vec3 triplanarWeights(vec3 normal) {
    vec3 blendWeight = normal * normal;
    blendWeight /= dot(blendWeight, vec3(1.0));
    return blendWeight;
}

// Sample texture using triplanar projection
vec4 triplanar(vec3 pos, vec3 normal, float scale, sampler2D tex) {
    vec2 uvX = pos.zy * scale;
    vec2 uvY = pos.xz * scale;
    vec2 uvZ = pos.xy * scale;

    vec4 colX = texture(tex, uvX);
    vec4 colY = texture(tex, uvY);
    vec4 colZ = texture(tex, uvZ);

    vec3 blendWeight = triplanarWeights(normal);
    return colX * blendWeight.x + colY * blendWeight.y + colZ * blendWeight.z;
}

// Triplanar sampling with UV offset for animation
vec4 triplanarOffset(vec3 pos, vec3 normal, float scale, vec3 offset, sampler2D tex) {
    vec3 scaledPos = pos * scale;

    vec4 colX = texture(tex, scaledPos.zy + offset.zy);
    vec4 colY = texture(tex, scaledPos.xz + offset.xz);
    vec4 colZ = texture(tex, scaledPos.xy + offset.xy);

    vec3 blendWeight = triplanarWeights(normal);
    return colX * blendWeight.x + colY * blendWeight.y + colZ * blendWeight.z;
}

// Reoriented Normal Mapping blend
// Combines two normal maps while preserving detail from both
// Reference: blog.selfshadow.com/publications/blending-in-detail/
vec3 blendRNM(vec3 n1, vec3 n2) {
    n1.z += 1.0;
    n2.xy = -n2.xy;
    return n1 * dot(n1, n2) / n1.z - n2;
}

// Triplanar normal mapping with proper tangent space handling
// Returns normal in object/world space matching input coordinate space
vec3 triplanarNormal(vec3 pos, vec3 normal, float scale, vec3 offset, sampler2D normalMap) {
    vec3 absNormal = abs(normal);

    // Higher power for sharper blend transitions on normals
    vec3 blendWeight = clamp(normal * normal * normal * normal, 0.0, 1.0);
    blendWeight /= dot(blendWeight, vec3(1.0));

    vec2 uvX = pos.zy * scale + offset.zy;
    vec2 uvY = pos.xz * scale + offset.xz;
    vec2 uvZ = pos.xy * scale + offset.xy;

    // Sample and unpack normals from [0,1] to [-1,1] range
    vec3 tangentNormalX = texture(normalMap, uvX).xyz * 2.0 - 1.0;
    vec3 tangentNormalY = texture(normalMap, uvY).xyz * 2.0 - 1.0;
    vec3 tangentNormalZ = texture(normalMap, uvZ).xyz * 2.0 - 1.0;

    // Swizzle and apply reoriented normal mapping blend
    tangentNormalX = blendRNM(vec3(normal.zy, absNormal.x), tangentNormalX);
    tangentNormalY = blendRNM(vec3(normal.xz, absNormal.y), tangentNormalY);
    tangentNormalZ = blendRNM(vec3(normal.xy, absNormal.z), tangentNormalZ);

    // Flip Z based on input normal sign for consistent orientation
    vec3 axisSign = sign(normal);
    tangentNormalX.z *= axisSign.x;
    tangentNormalY.z *= axisSign.y;
    tangentNormalZ.z *= axisSign.z;

    // Swizzle back to world space and blend
    vec3 outputNormal = normalize(
        tangentNormalX.zyx * blendWeight.x +
        tangentNormalY.xzy * blendWeight.y +
        tangentNormalZ.xyz * blendWeight.z
    );

    return outputNormal;
}

#endif // TRIPLANAR_GLSL
