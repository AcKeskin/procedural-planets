#version 450 core

// Space background vertex shader
// Generates fullscreen triangle and outputs view ray for sky rendering

out vec2 vTexCoord;
out vec3 vViewRay;

uniform mat4 uInvProjection;
uniform mat4 uInvView;

void main()
{
    // Generate fullscreen triangle covering NDC space
    float x = float((gl_VertexID & 1) << 2) - 1.0;
    float y = float((gl_VertexID & 2) << 1) - 1.0;

    vTexCoord = vec2((x + 1.0) * 0.5, (y + 1.0) * 0.5);

    // Calculate view ray for this vertex
    vec4 clipPos = vec4(x, y, 1.0, 1.0);
    vec4 viewPos = uInvProjection * clipPos;
    viewPos.xyz /= viewPos.w;
    vViewRay = normalize((uInvView * vec4(viewPos.xyz, 0.0)).xyz);

    gl_Position = vec4(x, y, 0.9999, 1.0); // Depth near far plane
}
