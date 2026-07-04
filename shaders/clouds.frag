#version 450 core

// lighting.glsl → dayFactor (night-side darkening, shared terminator).
// clouds.glsl → cloudCoverage (the same field the surface samples for the matching shadow).
#include "lighting.glsl"
#include "clouds.glsl"

in vec3 fragWorldPos;
in vec3 fragDir;

uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uSunColor;
uniform float uSunIntensity;
uniform float uAmbientLight;
uniform float uShellRadius;
uniform vec3 uColor;
uniform float uCoverage;
uniform float uDensity;
uniform float uScale;
uniform float uSpeed;
uniform float uTime;

out vec4 outColor;

void main()
{
    float cloud = cloudCoverage(fragDir, uScale, uCoverage, uSpeed, uTime);
    if (cloud < 0.01)
        discard;

    // Sun lighting: clouds are high-albedo, so the lit side reads near-white; the night side dims
    // to a dark silhouette. Same dayFactor curve the surface uses so the cloud terminator lines up
    // with the ground one. The wrap term (+0.4) keeps the day side bright rather than falling off
    // to grey as the sun grazes — clouds scatter light forward and sideways.
    vec3 lightDir = -normalize(uLightDir);
    float day = dayFactor(fragDir, lightDir, 0.12);
    float lambert = clamp(dot(fragDir, lightDir) * 0.5 + 0.5, 0.0, 1.0); // wrapped: soft, always lit on day side
    vec3 dayColor = uColor * uSunColor * (0.6 + 0.9 * lambert) * uSunIntensity;
    vec3 nightColor = uColor * uAmbientLight * 0.6;
    vec3 color = mix(nightColor, dayColor, day);

    float alpha = clamp(cloud * uDensity, 0.0, 1.0);

    // Soft layer crossing: fade the shell out as the camera nears it so descending THROUGH the layer
    // doesn't hit an opaque wall — the clouds thin to nothing right at the camera plane instead of
    // filling the screen. Scaled by shell radius so the fade band is altitude-appropriate.
    float camDist = length(uCameraPos - fragWorldPos);
    float fadeBand = uShellRadius * 0.15;
    alpha *= smoothstep(0.0, fadeBand, camDist);

    outColor = vec4(color, alpha);
}
