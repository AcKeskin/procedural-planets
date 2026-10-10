// Cube-sphere grid for the macro world field: 6 faces x N^2 cells stored as one
// 2D texture array (layer = face). Equal-angle mapping keeps cell areas within
// ~1.4x of each other, so the bake can treat cells as near-equal-area.
//
// Faces: 0 +X, 1 -X, 2 +Y, 3 -Y, 4 +Z, 5 -Z. A face point is n + a*u + b*v with
// a = tan(s * pi/4), b = tan(t * pi/4), (s, t) in [-1, 1].
//
// Cross-face neighbours need no orientation table: a cell index past the face edge
// is turned into its 3D direction on the extended face plane and re-projected,
// which lands on the right cell of the adjacent face by construction.
//
// Helpers are prefixed mg_ and define no PI/hash so they include cleanly next to
// shaders that inline their own.

const float MG_QUARTER_PI = 0.78539816339;

const vec3 MG_FACE_N[6] = vec3[6](
    vec3( 1, 0, 0), vec3(-1, 0, 0), vec3(0,  1, 0), vec3(0, -1, 0), vec3(0, 0,  1), vec3(0, 0, -1));
const vec3 MG_FACE_U[6] = vec3[6](
    vec3(0, 0, -1), vec3(0, 0,  1), vec3(1,  0, 0), vec3(1,  0, 0), vec3(1, 0,  0), vec3(-1, 0, 0));
const vec3 MG_FACE_V[6] = vec3[6](
    vec3(0, 1,  0), vec3(0, 1,  0), vec3(0,  0, -1), vec3(0,  0, 1), vec3(0, 1,  0), vec3(0,  1, 0));

int mg_dirToFace(vec3 d)
{
    vec3 a = abs(d);
    if (a.x >= a.y && a.x >= a.z) return d.x >= 0.0 ? 0 : 1;
    if (a.y >= a.z)               return d.y >= 0.0 ? 2 : 3;
    return d.z >= 0.0 ? 4 : 5;
}

// Equal-angle face coordinates (s, t) of d on face f. Valid for any d in front of f.
vec2 mg_dirToST(vec3 d, int f)
{
    float w = dot(d, MG_FACE_N[f]);
    vec2 ab = vec2(dot(d, MG_FACE_U[f]), dot(d, MG_FACE_V[f])) / w;
    return atan(ab) / MG_QUARTER_PI;
}

vec3 mg_faceSTToDir(int f, vec2 st)
{
    vec2 ab = tan(st * MG_QUARTER_PI);
    return normalize(MG_FACE_N[f] + ab.x * MG_FACE_U[f] + ab.y * MG_FACE_V[f]);
}

// Centre direction of cell ij on face f. ij may lie outside [0, n): the point
// then sits on the extended face plane, past the edge.
vec3 mg_cellDir(int f, ivec2 ij, int n)
{
    vec2 st = (vec2(ij) + 0.5) / float(n) * 2.0 - 1.0;
    return mg_faceSTToDir(f, st);
}

// (face, i, j) of the cell containing d.
ivec3 mg_dirToCell(vec3 d, int n)
{
    int f = mg_dirToFace(d);
    vec2 st = mg_dirToST(d, f);
    ivec2 ij = clamp(ivec2(floor((st * 0.5 + 0.5) * float(n))), ivec2(0), ivec2(n - 1));
    return ivec3(f, ij);
}

// Resolve a possibly out-of-face index to a real cell. Diagonal taps past a cube
// corner (three faces meet, no fourth cell) land on the nearest existing cell.
ivec3 mg_wrapCell(int f, ivec2 ij, int n)
{
    if (all(greaterThanEqual(ij, ivec2(0))) && all(lessThan(ij, ivec2(n))))
        return ivec3(f, ij);
    return mg_dirToCell(mg_cellDir(f, ij, n), n);
}

ivec3 mg_neighbour(int f, ivec2 ij, int dx, int dy, int n)
{
    return mg_wrapCell(f, ij + ivec2(dx, dy), n);
}

// Bilinear sample by direction. Taps past a face edge come from the adjacent face,
// so the result is continuous across cube edges (hardware filtering stops at layers).
vec4 mg_sample(sampler2DArray field, vec3 d)
{
    int n = textureSize(field, 0).x;
    int f = mg_dirToFace(d);
    vec2 x = (mg_dirToST(d, f) * 0.5 + 0.5) * float(n) - 0.5;
    ivec2 i0 = ivec2(floor(x));
    vec2 w = x - vec2(i0);

    ivec3 c00 = mg_wrapCell(f, i0,               n);
    ivec3 c10 = mg_wrapCell(f, i0 + ivec2(1, 0), n);
    ivec3 c01 = mg_wrapCell(f, i0 + ivec2(0, 1), n);
    ivec3 c11 = mg_wrapCell(f, i0 + ivec2(1, 1), n);

    vec4 v00 = texelFetch(field, ivec3(c00.yz, c00.x), 0);
    vec4 v10 = texelFetch(field, ivec3(c10.yz, c10.x), 0);
    vec4 v01 = texelFetch(field, ivec3(c01.yz, c01.x), 0);
    vec4 v11 = texelFetch(field, ivec3(c11.yz, c11.x), 0);

    return mix(mix(v00, v10, w.x), mix(v01, v11, w.x), w.y);
}
