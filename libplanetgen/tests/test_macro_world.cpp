#include <catch2/catch_test_macros.hpp>

#include "planetgen/planetgen.h"
#include "api/InternalTypes.h"
#include "strategy/GenerationPipeline.h"
#include "strategy/ContinentMaskPass.h"
#include "strategy/MacroWorldPass.h"
#include "planetgen/model/BodyConfig.h"
#include "planetgen/model/BodyConfigJson.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// macro-world-layer proofs. All cases carry "macro_world" in the name so
// `ctest -R macro_world` selects them. GPU work goes through the library's own
// backend (ctx->backend); the test exe makes no raw GL calls.

namespace
{

PgContext MakeHeadlessCtx()
{
    PgContextDesc d{};
    d.use_existing_context = 0;
    return pg_context_create(&d);
}

std::vector<float> MakeSphereVertices(int n)
{
    std::vector<float> v;
    v.reserve(n * 3);
    const float goldenAngle = 2.39996323f;
    for (int i = 0; i < n; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(n);
        const float inc = std::acos(1.0f - 2.0f * t);
        const float az = goldenAngle * static_cast<float>(i);
        v.push_back(std::sin(inc) * std::cos(az));
        v.push_back(std::cos(inc));
        v.push_back(std::sin(inc) * std::sin(az));
    }
    return v;
}

uint64_t Fnv1a(uint64_t h, const void* data, size_t bytes)
{
    const auto* p = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < bytes; ++i)
    {
        h ^= p[i];
        h *= 0x100000001b3ull;
    }
    return h;
}

planetgen::BodyConfig LoadEarth()
{
    planetgen::BodyConfig cfg;
    const std::string path = std::string(PLANETGEN_TEST_DATA_DIR) + "/data/bodies/earth.json";
    REQUIRE(planetgen::BodyConfigLoadFile(path, cfg).empty());
    return cfg;
}

// Hash of the whole-mesh Generate() output (heights + normals + shading).
uint64_t HashGenerate(PgContext ctx, const planetgen::BodyConfig& cfg, uint32_t seed)
{
    const int N = 4096;
    const auto verts = MakeSphereVertices(N);

    planetgen::GpuTextureHandle maskTex = 0;
    uint64_t maskKey = 0;
    planetgen::GenerationPipeline pipeline(*ctx->backend, ctx->registry, ctx->palettes);
    auto r = pipeline.Generate(cfg, verts.data(), N, seed, { &maskTex, &maskKey });
    REQUIRE(r.count == static_cast<uint32_t>(N));

    uint64_t h = 0xcbf29ce484222325ull;
    h = Fnv1a(h, r.heights.data(), r.heights.size() * sizeof(float));
    h = Fnv1a(h, r.normals.data(), r.normals.size() * sizeof(float));
    h = Fnv1a(h, r.shading.data(), r.shading.size() * sizeof(float));

    auto& gpu = *ctx->backend;
    gpu.DestroyBuffer(r.heightsGpuBuffer);
    gpu.DestroyBuffer(r.normalsGpuBuffer);
    gpu.DestroyBuffer(r.shadingGpuBuffer);
    if (maskTex)
        gpu.DestroyTexture(maskTex);
    return h;
}

// Test-only probe shaders are built from the on-disk include so they exercise
// exactly the macro_grid.glsl the library embeds.
std::string ReadShaderFile(const std::string& relPath)
{
    std::ifstream f(std::string(PLANETGEN_TEST_DATA_DIR) + "/" + relPath);
    REQUIRE(f.is_open());
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

uint32_t CompileProbe(PgContext ctx, const std::string& key, const std::string& body)
{
    const std::string src = "#version 450\n" + ReadShaderFile("shaders/includes/macro_grid.glsl") + body;
    REQUIRE(ctx->registry.RegisterProgram(key, src));
    return ctx->registry.Resolve(key);
}

struct Vec3
{
    double x, y, z;
};

double Angle(const Vec3& a, const Vec3& b)
{
    const double d = a.x * b.x + a.y * b.y + a.z * b.z;
    return std::acos(std::clamp(d, -1.0, 1.0));
}

Vec3 Normalize(Vec3 v)
{
    const double l = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return { v.x / l, v.y / l, v.z / l };
}

// Writes each cell's direction (image, binding 0) and the linear ids of its
// +x, -x, +y, -y neighbours (SSBO, binding 1). Linear id = (face * n + j) * n + i.
const char* kNeighbourProbe = R"(
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2DArray dirImg;
layout(std430, binding = 1) writeonly buffer Neighbours { ivec4 nb[]; };
int lin(ivec3 c, int n) { return (c.x * n + c.z) * n + c.y; }
void main()
{
    int n = imageSize(dirImg).x;
    ivec2 ij = ivec2(gl_GlobalInvocationID.xy);
    int f = int(gl_GlobalInvocationID.z);
    imageStore(dirImg, ivec3(ij, f), vec4(mg_cellDir(f, ij, n), 0.0));
    nb[lin(ivec3(f, ij), n)] = ivec4(lin(mg_neighbour(f, ij,  1,  0, n), n),
                                     lin(mg_neighbour(f, ij, -1,  0, n), n),
                                     lin(mg_neighbour(f, ij,  0,  1, n), n),
                                     lin(mg_neighbour(f, ij,  0, -1, n), n));
}
)";

// Fills the field with a smooth analytic function of the cell direction.
const char* kFieldFill = R"(
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2DArray fieldImg;
void main()
{
    int n = imageSize(fieldImg).x;
    ivec2 ij = ivec2(gl_GlobalInvocationID.xy);
    int f = int(gl_GlobalInvocationID.z);
    vec3 d = mg_cellDir(f, ij, n);
    imageStore(fieldImg, ivec3(ij, f), vec4(d.x + 0.5 * d.y * d.z, sin(3.0 * d.y), d.z, 1.0));
}
)";

// Samples the field (sampler unit 5) at directions from an SSBO. w >= 0 forces
// the face the sample is taken from; w < 0 picks it from the direction.
const char* kSampleProbe = R"(
layout(local_size_x = 64) in;
layout(binding = 5) uniform sampler2DArray field;
layout(std430, binding = 0) readonly buffer Dirs { vec4 dirs[]; };
layout(std430, binding = 1) writeonly buffer Values { vec4 values[]; };
void main()
{
    uint id = gl_GlobalInvocationID.x;
    if (id >= uint(dirs.length())) return;
    vec3 d = normalize(dirs[id].xyz);
    int face = int(dirs[id].w);
    values[id] = face >= 0 ? mg_sampleOnFace(field, d, face) : mg_sample(field, d);
}
)";

// A hand-built elevation field: ocean everywhere except an equatorial continent
// (|lat| < 25 deg, lon 0..70 deg) with a tall north-south ridge at lon 35 deg.
const char* kRidgeField = R"(
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2DArray fieldImg;
void main()
{
    int n = imageSize(fieldImg).x;
    ivec2 ij = ivec2(gl_GlobalInvocationID.xy);
    int f = int(gl_GlobalInvocationID.z);
    vec3 d = mg_cellDir(f, ij, n);
    float lat = degrees(asin(d.y));
    float lon = degrees(atan(d.z, d.x));
    float elev = -0.5;
    if (abs(lat) < 25.0 && lon > 0.0 && lon < 70.0)
        elev = 0.05 + 0.8 * exp(-pow((lon - 35.0) / 3.0, 2.0));
    imageStore(fieldImg, ivec3(ij, f), vec4(elev, 0.0, 0.0, 0.0));
}
)";

// Cell directions (vec4 per cell) and +x/-x/+y/-y neighbour ids for a grid size.
struct GridLayout
{
    std::vector<float> dirs;
    std::vector<int32_t> neighbours;
};

GridLayout ReadGridLayout(PgContext ctx, int n)
{
    auto& gpu = *ctx->backend;
    const uint32_t program = CompileProbe(ctx, "test_macro_grid_neighbours", kNeighbourProbe);
    const auto dirTex = gpu.CreateTexture2DArray_RGBA32F(n, 6);
    const size_t cells = static_cast<size_t>(6) * n * n;
    const auto nbBuf = gpu.CreateBuffer(cells * 4 * sizeof(int32_t));

    gpu.BindShader(program);
    gpu.BindImage2DArray(dirTex, 0);
    gpu.BindBuffer(nbBuf, 1);
    gpu.Dispatch((n + 7) / 8, (n + 7) / 8, 6);
    gpu.ImageBarrier();
    gpu.Barrier();

    GridLayout g;
    g.dirs.resize(cells * 4);
    g.neighbours.resize(cells * 4);
    gpu.DownloadTexture(dirTex, g.dirs.data(), g.dirs.size() * sizeof(float));
    gpu.DownloadBuffer(nbBuf, g.neighbours.data(), g.neighbours.size() * sizeof(int32_t));
    gpu.DestroyBuffer(nbBuf);
    gpu.DestroyTexture(dirTex);
    return g;
}

// Points exactly on each of the 12 cube edges (kept away from the corners), each
// emitted twice: sampled from either adjacent face (w = face index).
std::vector<float> EdgePairDirs()
{
    std::vector<float> dirs;
    for (int a = 0; a < 3; ++a)
        for (int b = a + 1; b < 3; ++b)
        {
            const int c = 3 - a - b;
            for (int sa = -1; sa <= 1; sa += 2)
                for (int sb = -1; sb <= 1; sb += 2)
                    for (int k = 0; k <= 32; ++k)
                    {
                        double p[3] = {};
                        p[a] = sa;
                        p[b] = sb;
                        p[c] = -0.9 + 1.8 * k / 32.0;
                        const Vec3 d = Normalize({ p[0], p[1], p[2] });
                        const float faceA = float(a * 2 + (sa > 0 ? 0 : 1));
                        const float faceB = float(b * 2 + (sb > 0 ? 0 : 1));
                        dirs.insert(dirs.end(), { float(d.x), float(d.y), float(d.z), faceA });
                        dirs.insert(dirs.end(), { float(d.x), float(d.y), float(d.z), faceB });
                    }
        }
    return dirs;
}

// mg_sample the field at each direction (vec4 per entry) on the GPU.
std::vector<float> SampleField(PgContext ctx, planetgen::GpuTextureHandle field, const std::vector<float>& dirs)
{
    auto& gpu = *ctx->backend;
    const uint32_t sample = CompileProbe(ctx, "test_macro_grid_sample", kSampleProbe);
    const size_t count = dirs.size() / 4;

    const auto dirBuf = gpu.CreateBuffer(dirs.size() * sizeof(float));
    gpu.UploadBuffer(dirBuf, dirs.data(), dirs.size() * sizeof(float));
    const auto valBuf = gpu.CreateBuffer(count * 4 * sizeof(float));

    gpu.BindShader(sample);
    gpu.BindTexture2DArray(field, 5);
    gpu.BindBuffer(dirBuf, 0);
    gpu.BindBuffer(valBuf, 1);
    gpu.Dispatch(static_cast<uint32_t>((count + 63) / 64));
    gpu.Barrier();

    std::vector<float> vals(count * 4);
    gpu.DownloadBuffer(valBuf, vals.data(), vals.size() * sizeof(float));
    gpu.DestroyBuffer(dirBuf);
    gpu.DestroyBuffer(valBuf);
    return vals;
}

std::vector<float> DownloadField(PgContext ctx, planetgen::GpuTextureHandle field, uint32_t res)
{
    std::vector<float> texels(static_cast<size_t>(res) * res * 6 * 4);
    ctx->backend->DownloadTexture(field, texels.data(), texels.size() * sizeof(float));
    return texels;
}

planetgen::BodyConfig MacroEarth(int resolution)
{
    auto cfg = LoadEarth();
    cfg.macroWorld.enabled = true;
    cfg.macroWorld.resolution = resolution;
    return cfg;
}

// Owns a continent mask for a config so bake tests can run stages directly.
struct MaskHolder
{
    PgContext ctx;
    planetgen::GpuTextureHandle tex = 0;
    MaskHolder(PgContext c, const planetgen::BodyConfig& cfg, uint32_t seed) : ctx(c)
    {
        tex = planetgen::BakeContinentMask(*ctx->backend, ctx->registry, cfg, seed);
        REQUIRE(tex != 0);
    }
    ~MaskHolder() { ctx->backend->DestroyTexture(tex); }
};

} // namespace

TEST_CASE("macro_world off: Earth output matches the pre-macro baseline", "[gpu][macro_world]")
{
    // Dev-GPU baseline. With the layer off the height + shading output must stay
    // byte-identical to it. Re-recorded for ground v1 (fine relief octaves, Earth normal
    // step 0.0008, heightScale 0.03). A different GPU/driver can legitimately produce
    // other bits; re-record there.
    constexpr uint64_t kBaseline = 0xab0bdea12dc3aaf1ull;

    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);

    auto cfg = LoadEarth();
    cfg.macroWorld.enabled = false; // explicit: this proves the off path whatever earth.json ships

    const uint64_t h = HashGenerate(ctx, cfg, 42u);
    std::printf("macro_world off-parity hash: 0x%016llx\n", static_cast<unsigned long long>(h));
    CHECK(h == kBaseline);

    pg_context_destroy(ctx);
}

TEST_CASE("macro_world grid: cross-face neighbours are adjacent and reciprocal", "[gpu][macro_world]")
{
    const int n = 32;
    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);
    auto& gpu = *ctx->backend;

    const uint32_t program = CompileProbe(ctx, "test_macro_grid_neighbours", kNeighbourProbe);
    const auto dirTex = gpu.CreateTexture2DArray_RGBA32F(n, 6);
    const size_t cells = static_cast<size_t>(6) * n * n;
    const auto nbBuf = gpu.CreateBuffer(cells * 4 * sizeof(int32_t));

    gpu.BindShader(program);
    gpu.BindImage2DArray(dirTex, 0);
    gpu.BindBuffer(nbBuf, 1);
    gpu.Dispatch(n / 8, n / 8, 6);
    gpu.ImageBarrier();
    gpu.Barrier();

    std::vector<float> dirs(cells * 4);
    std::vector<int32_t> nb(cells * 4);
    gpu.DownloadTexture(dirTex, dirs.data(), dirs.size() * sizeof(float));
    gpu.DownloadBuffer(nbBuf, nb.data(), nb.size() * sizeof(int32_t));

    auto dirOf = [&](size_t c) { return Vec3{ dirs[c * 4], dirs[c * 4 + 1], dirs[c * 4 + 2] }; };
    const double cellAngle = (3.14159265358979 * 0.5) / n;

    int crossings = 0;
    for (int f = 0; f < 6; ++f)
        for (int j = 0; j < n; ++j)
            for (int i = 0; i < n; ++i)
            {
                const bool edgeI = (i == 0 || i == n - 1);
                const bool edgeJ = (j == 0 || j == n - 1);
                if (!(edgeI || edgeJ) || (edgeI && edgeJ))
                    continue; // interior cells never cross; corner cells are the documented approximation

                const size_t c = (static_cast<size_t>(f) * n + j) * n + i;
                const bool outward[4] = { i == n - 1, i == 0, j == n - 1, j == 0 };
                for (int k = 0; k < 4; ++k)
                {
                    if (!outward[k])
                        continue;
                    const size_t other = static_cast<size_t>(nb[c * 4 + k]);
                    INFO("face " << f << " cell " << i << "," << j << " dir " << k << " -> " << other);
                    REQUIRE(other < cells);
                    CHECK(static_cast<int>(other / (n * n)) != f);
                    CHECK(Angle(dirOf(c), dirOf(other)) < 1.5 * cellAngle);

                    const int32_t* back = &nb[other * 4];
                    const bool reciprocal = std::find(back, back + 4, static_cast<int32_t>(c)) != back + 4;
                    CHECK(reciprocal);
                    ++crossings;
                }
            }
    CHECK(crossings == 6 * 4 * (n - 2));

    gpu.DestroyBuffer(nbBuf);
    gpu.DestroyTexture(dirTex);
    pg_context_destroy(ctx);
}

TEST_CASE("macro_world grid: bilinear sampling is continuous across cube edges", "[gpu][macro_world]")
{
    const int n = 32;
    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);
    auto& gpu = *ctx->backend;

    const uint32_t fill = CompileProbe(ctx, "test_macro_grid_fill", kFieldFill);

    const auto field = gpu.CreateTexture2DArray_RGBA32F(n, 6);
    gpu.BindShader(fill);
    gpu.BindImage2DArray(field, 0);
    gpu.Dispatch(n / 8, n / 8, 6);
    gpu.ImageBarrier();

    const std::vector<float> dirs = EdgePairDirs();
    const size_t count = dirs.size() / 4;
    const std::vector<float> vals = SampleField(ctx, field, dirs);

    double worstSeam = 0.0;
    double worstAnalytic = 0.0;
    for (size_t p = 0; p < count; p += 2)
    {
        for (int ch = 0; ch < 3; ++ch)
            worstSeam = std::max(worstSeam, double(std::fabs(vals[p * 4 + ch] - vals[(p + 1) * 4 + ch])));

        const Vec3 d{ dirs[p * 4], dirs[p * 4 + 1], dirs[p * 4 + 2] };
        worstAnalytic = std::max(worstAnalytic, std::fabs(vals[p * 4] - (d.x + 0.5 * d.y * d.z)));
    }
    INFO("worst seam jump " << worstSeam << ", worst analytic error " << worstAnalytic);
    CHECK(worstSeam < 1e-5);
    CHECK(worstAnalytic < 0.02);

    gpu.DestroyTexture(field);
    pg_context_destroy(ctx);
}

TEST_CASE("macro_world bake: same seed gives a bit-identical field, another seed differs", "[gpu][macro_world]")
{
    const int Res = 64;
    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);
    auto& gpu = *ctx->backend;

    const auto cfg = MacroEarth(Res);
    {
    MaskHolder mask(ctx, cfg, 42u);

    const auto a = planetgen::BakeMacroWorld(gpu, ctx->registry, cfg, 42u, mask.tex);
    const auto b = planetgen::BakeMacroWorld(gpu, ctx->registry, cfg, 42u, mask.tex);
    const auto c = planetgen::BakeMacroWorld(gpu, ctx->registry, cfg, 7u, mask.tex);
    REQUIRE(a != 0);
    REQUIRE(b != 0);
    REQUIRE(c != 0);

    const auto fa = DownloadField(ctx, a, Res);
    const auto fb = DownloadField(ctx, b, Res);
    const auto fc = DownloadField(ctx, c, Res);
    CHECK(std::memcmp(fa.data(), fb.data(), fa.size() * sizeof(float)) == 0);
    CHECK(std::memcmp(fa.data(), fc.data(), fa.size() * sizeof(float)) != 0);

    for (auto t : { a, b, c })
        gpu.DestroyTexture(t);
    }
    pg_context_destroy(ctx);
}

TEST_CASE("macro_world bake: field is seamless across cube-face edges", "[gpu][macro_world]")
{
    const int Res = 128;
    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);
    auto& gpu = *ctx->backend;

    const auto cfg = MacroEarth(Res);
    {
    MaskHolder mask(ctx, cfg, 42u);
    const auto field = planetgen::BakeMacroWorld(gpu, ctx->registry, cfg, 42u, mask.tex);
    REQUIRE(field != 0);

    const auto dirs = EdgePairDirs();
    const auto vals = SampleField(ctx, field, dirs);

    double worst = 0.0;
    for (size_t p = 0; p < vals.size() / 4; p += 2)
        for (int ch = 0; ch < 4; ++ch)
        {
            const double a = vals[p * 4 + ch];
            const double b = vals[(p + 1) * 4 + ch];
            worst = std::max(worst, std::fabs(a - b) / std::max(1.0, std::fabs(a)));
        }
    INFO("worst relative seam jump " << worst);
    CHECK(worst < 1e-4);

    gpu.DestroyTexture(field);
    }
    pg_context_destroy(ctx);
}

TEST_CASE("macro_world erosion: lowers land and conserves height + sediment", "[gpu][macro_world]")
{
    const int Res = 128;
    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);
    {
        const auto cfg = MacroEarth(Res);
        MaskHolder mask(ctx, cfg, 42u);

        planetgen::MacroWorldBake bake(*ctx->backend, ctx->registry, cfg, 42u);
        REQUIRE(bake.Plates(mask.tex));
        const auto before = DownloadField(ctx, bake.Geo(), Res);
        REQUIRE(bake.Erode(cfg.macroWorld.erosionIterations));
        const auto after = DownloadField(ctx, bake.Geo(), Res);

        double landBefore = 0.0, landAfter = 0.0, massBefore = 0.0, massAfter = 0.0, scale = 0.0;
        double deepestCut = 0.0;
        int landCells = 0;
        for (size_t c = 0; c < before.size() / 4; ++c)
        {
            const double hb = before[c * 4], ha = after[c * 4];
            massBefore += hb + before[c * 4 + 2];
            massAfter += ha + after[c * 4 + 2];
            scale += std::fabs(hb);
            if (hb > 0.0)
            {
                landBefore += hb;
                landAfter += ha;
                deepestCut = std::max(deepestCut, hb - ha);
                ++landCells;
            }
        }
        REQUIRE(landCells > 0);
        INFO("mean land height " << landBefore / landCells << " -> " << landAfter / landCells
             << " m, deepest cut " << deepestCut << " m, mass drift " << (massAfter - massBefore));
        CHECK(landAfter < landBefore);
        CHECK(deepestCut > 50.0);
        CHECK(std::fabs(massAfter - massBefore) / scale < 1e-5);
    }
    pg_context_destroy(ctx);
}

TEST_CASE("macro_world rivers: every land cell drains to the ocean without cycles", "[gpu][macro_world]")
{
    const int Res = 128;
    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);
    {
        const auto cfg = MacroEarth(Res);
        MaskHolder mask(ctx, cfg, 42u);

        planetgen::MacroWorldBake bake(*ctx->backend, ctx->registry, cfg, 42u);
        REQUIRE(bake.RunAll(mask.tex));
        const auto drain = DownloadField(ctx, bake.Drainage(), Res);
        const auto field = DownloadField(ctx, bake.Field(), Res);

        const size_t cells = drain.size() / 4;
        auto isOcean = [&](size_t c) { return drain[c * 4 + 1] != 0.0f; };

        // 0 = unknown, 1 = on the current walk, 2 = reaches the ocean.
        std::vector<uint8_t> state(cells, 0);
        std::vector<size_t> walk;
        size_t landCells = 0, badCells = 0, riverCells = 0;
        float maxRiver = 0.0f;
        for (size_t start = 0; start < cells; ++start)
        {
            if (isOcean(start))
                continue;
            ++landCells;
            maxRiver = std::max(maxRiver, field[start * 4 + 1]);
            if (field[start * 4 + 1] > 4.0f) // > ~15 000 km^2 upstream: a river
                ++riverCells;

            walk.clear();
            size_t c = start;
            bool ok = false;
            for (size_t step = 0; step <= cells; ++step)
            {
                if (isOcean(c) || state[c] == 2)
                {
                    ok = true;
                    break;
                }
                if (state[c] == 1)
                    break; // cycle
                state[c] = 1;
                walk.push_back(c);
                const auto next = static_cast<size_t>(drain[c * 4]);
                if (next >= cells || next == c)
                    break; // land sink
                c = next;
            }
            for (size_t w : walk)
                state[w] = ok ? 2 : 0;
            if (!ok)
                ++badCells;
        }

        INFO(landCells << " land cells, " << badCells << " fail to reach the ocean, "
             << riverCells << " river cells, max river flux " << maxRiver);
        CHECK(landCells > 0);
        CHECK(badCells == 0);
        CHECK(riverCells > 0);
    }
    pg_context_destroy(ctx);
}

TEST_CASE("macro_world climate: the leeward side of a ridge is drier than the windward side", "[gpu][macro_world]")
{
    const int Res = 128;
    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);
    auto& gpu = *ctx->backend;
    {
        const auto cfg = MacroEarth(Res);
        const uint32_t ridge = CompileProbe(ctx, "test_macro_ridge_field", kRidgeField);
        const auto field = gpu.CreateTexture2DArray_RGBA32F(Res, 6);
        gpu.BindShader(ridge);
        gpu.BindImage2DArray(field, 0);
        gpu.Dispatch(Res / 8, Res / 8, 6);
        gpu.ImageBarrier();

        planetgen::MacroWorldBake bake(gpu, ctx->registry, cfg, 42u);
        REQUIRE(bake.Climate(field));
        const auto values = DownloadField(ctx, field, Res);
        const auto grid = ReadGridLayout(ctx, Res);

        // Trade winds near the equator blow toward increasing longitude here, so the
        // windward slope is west of lon 35 and the leeward slope east of it.
        double windward = 0.0, leeward = 0.0;
        int nw = 0, nl = 0;
        for (size_t c = 0; c < values.size() / 4; ++c)
        {
            const double lat = std::asin(grid.dirs[c * 4 + 1]) * 57.29578;
            const double lon = std::atan2(grid.dirs[c * 4 + 2], grid.dirs[c * 4]) * 57.29578;
            if (std::fabs(lat) > 12.0)
                continue;
            if (lon > 24.0 && lon < 31.0) { windward += values[c * 4 + 2]; ++nw; }
            if (lon > 39.0 && lon < 46.0) { leeward += values[c * 4 + 2]; ++nl; }
        }
        REQUIRE(nw > 0);
        REQUIRE(nl > 0);
        INFO("windward moisture " << windward / nw << ", leeward " << leeward / nl);
        CHECK(leeward / nl < windward / nw);

        gpu.DestroyTexture(field);
    }
    pg_context_destroy(ctx);
}

TEST_CASE("macro_world climate: coastal land is wetter than continental interiors", "[gpu][macro_world]")
{
    const int Res = 128;
    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);
    {
        const auto cfg = MacroEarth(Res);
        MaskHolder mask(ctx, cfg, 42u);
        const auto field = planetgen::BakeMacroWorld(*ctx->backend, ctx->registry, cfg, 42u, mask.tex);
        REQUIRE(field != 0);
        const auto values = DownloadField(ctx, field, Res);
        const auto grid = ReadGridLayout(ctx, Res);
        const size_t cells = values.size() / 4;

        // Distance to the nearest ocean cell in grid steps (BFS over 4-neighbours).
        std::vector<int> dist(cells, -1);
        std::deque<size_t> queue;
        for (size_t c = 0; c < cells; ++c)
            if (values[c * 4] < 0.0f)
            {
                dist[c] = 0;
                queue.push_back(c);
            }
        while (!queue.empty())
        {
            const size_t c = queue.front();
            queue.pop_front();
            for (int k = 0; k < 4; ++k)
            {
                const size_t nb = static_cast<size_t>(grid.neighbours[c * 4 + k]);
                if (dist[nb] < 0)
                {
                    dist[nb] = dist[c] + 1;
                    queue.push_back(nb);
                }
            }
        }

        double coastal = 0.0, interior = 0.0;
        int nc = 0, ni = 0;
        for (size_t c = 0; c < cells; ++c)
        {
            if (std::fabs(grid.dirs[c * 4 + 1]) > 0.85) // skip polar caps
                continue;
            if (dist[c] >= 1 && dist[c] <= 2) { coastal += values[c * 4 + 2]; ++nc; }
            if (dist[c] >= 6) { interior += values[c * 4 + 2]; ++ni; }
        }
        REQUIRE(nc > 0);
        REQUIRE(ni > 0);
        INFO("coastal moisture " << coastal / nc << " (" << nc << " cells), interior "
             << interior / ni << " (" << ni << " cells)");
        CHECK(coastal / nc > interior / ni);

        ctx->backend->DestroyTexture(field);
    }
    pg_context_destroy(ctx);
}

namespace
{
// Unit vectors on a lat/lon grid. Edge vertices are computed from the same
// (lat, lon) inputs in both patches, so shared-edge directions are bit-equal.
std::vector<float> LatLonPatch(double lat0, double lon0, double span, int steps)
{
    std::vector<float> v;
    for (int a = 0; a <= steps; ++a)
        for (int b = 0; b <= steps; ++b)
        {
            const double lat = (lat0 + span * a / steps) * 0.0174532925199;
            const double lon = (lon0 + span * b / steps) * 0.0174532925199;
            v.push_back(float(std::cos(lat) * std::cos(lon)));
            v.push_back(float(std::sin(lat)));
            v.push_back(float(std::cos(lat) * std::sin(lon)));
        }
    return v;
}
} // namespace

TEST_CASE("macro_world patches: adjacent patches match on their shared edge with the layer on", "[gpu][macro_world][patch]")
{
    const int Steps = 16;
    const uint32_t N = (Steps + 1) * (Steps + 1);
    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);
    auto& gpu = *ctx->backend;
    {
        const auto cfg = MacroEarth(128);
        planetgen::GpuTextureHandle maskTex = 0, macroTex = 0;
        uint64_t maskKey = 0, macroKey = 0;
        planetgen::GenerationPipeline pipeline(gpu, ctx->registry, ctx->palettes);

        // Patch A spans lon [20, 30], patch B lon [30, 40]: A's last column is B's first.
        auto run = [&](const std::vector<float>& verts)
        {
            const auto h = gpu.CreateBuffer(N * sizeof(float));
            const auto nb = gpu.CreateBuffer(N * 3 * sizeof(float));
            const auto sh = gpu.CreateBuffer(N * 4 * sizeof(float));
            REQUIRE(pipeline.GeneratePatch(cfg, verts.data(), N, 42u, h, nb, sh,
                                           { &maskTex, &maskKey }, { &macroTex, &macroKey }));
            std::vector<float> out(N * 8);
            gpu.DownloadBuffer(h, out.data(), N * sizeof(float));
            gpu.DownloadBuffer(nb, out.data() + N, N * 3 * sizeof(float));
            gpu.DownloadBuffer(sh, out.data() + N * 4, N * 4 * sizeof(float));
            for (auto b : { h, nb, sh })
                gpu.DestroyBuffer(b);
            return out;
        };
        const auto a = run(LatLonPatch(-5.0, 20.0, 10.0, Steps));
        REQUIRE(macroTex != 0);
        const auto b = run(LatLonPatch(-5.0, 30.0, 10.0, Steps));

        int mismatches = 0;
        for (int row = 0; row <= Steps; ++row)
        {
            const uint32_t va = row * (Steps + 1) + Steps; // A: last column
            const uint32_t vb = row * (Steps + 1);         // B: first column
            mismatches += a[va] != b[vb];
            for (int k = 0; k < 3; ++k)
                mismatches += a[N + va * 3 + k] != b[N + vb * 3 + k];
            for (int k = 0; k < 4; ++k)
                mismatches += a[N * 4 + va * 4 + k] != b[N * 4 + vb * 4 + k];
        }
        CHECK(mismatches == 0);

        gpu.DestroyTexture(maskTex);
        gpu.DestroyTexture(macroTex);
    }
    pg_context_destroy(ctx);
}

TEST_CASE("macro_world on: height and moisture come from the baked field, off restores the baseline", "[gpu][macro_world]")
{
    const int N = 4096;
    const auto verts = MakeSphereVertices(N);
    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);
    auto& gpu = *ctx->backend;
    {
        planetgen::GpuTextureHandle maskTex = 0, macroTex = 0;
        uint64_t maskKey = 0, macroKey = 0;
        planetgen::GenerationPipeline pipeline(gpu, ctx->registry, ctx->palettes);
        auto run = [&](const planetgen::BodyConfig& cfg)
        {
            auto r = pipeline.Generate(cfg, verts.data(), N, 42u, { &maskTex, &maskKey }, { &macroTex, &macroKey });
            REQUIRE(r.count == static_cast<uint32_t>(N));
            gpu.DestroyBuffer(r.heightsGpuBuffer);
            gpu.DestroyBuffer(r.normalsGpuBuffer);
            gpu.DestroyBuffer(r.shadingGpuBuffer);
            return r;
        };

        auto off = LoadEarth();
        off.macroWorld.enabled = false;
        auto on = MacroEarth(128);
        const auto a = run(off);
        const auto b = run(on);
        REQUIRE(macroTex != 0);
        const auto c = run(off); // same body caches, layer switched back off

        int heightDiffs = 0, moistureDiffs = 0;
        for (int i = 0; i < N; ++i)
        {
            heightDiffs += a.heights[i] != b.heights[i];
            moistureDiffs += a.shading[i * 4 + 1] != b.shading[i * 4 + 1];
        }
        CHECK(heightDiffs > N / 2);
        CHECK(moistureDiffs > N / 2);
        CHECK(std::memcmp(a.heights.data(), c.heights.data(), N * sizeof(float)) == 0);
        CHECK(std::memcmp(a.shading.data(), c.shading.data(), N * 4 * sizeof(float)) == 0);

        gpu.DestroyTexture(maskTex);
        gpu.DestroyTexture(macroTex);
    }
    pg_context_destroy(ctx);
}

TEST_CASE("macro_world programs: every macro pass and shader variant compiles", "[gpu][macro_world]")
{
    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);
    CHECK(pg_context_get_error(ctx) == PG_SUCCESS);

    using BP = planetgen::BuiltinProgram;
    for (BP id : { BP::MacroPlates, BP::MacroErosionRainfall, BP::MacroErosionFlow, BP::MacroErosionSediment,
                   BP::MacroDrainageFlux, BP::MacroFill, BP::MacroRivers, BP::MacroClimate,
                   BP::HeightMacro, BP::ShadingEarthMacro, BP::HeightStyle, BP::HeightMacroStyle })
    {
        INFO("program " << planetgen::BuiltinKey(id));
        CHECK(ctx->registry.Resolve(id) != 0);
    }
    pg_context_destroy(ctx);
}

TEST_CASE("macro_world timing: the full bake at the default resolution takes under 5 s", "[gpu][macro_world]")
{
    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);
    auto& gpu = *ctx->backend;
    {
        auto cfg = LoadEarth();
        cfg.macroWorld.enabled = true; // every other macro param at its default
        MaskHolder mask(ctx, cfg, 42u);
        gpu.Finish(); // the mask bake is not part of the timed macro bake

        const auto t0 = std::chrono::steady_clock::now();
        const auto field = planetgen::BakeMacroWorld(gpu, ctx->registry, cfg, 42u, mask.tex);
        gpu.Finish();
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        REQUIRE(field != 0);

        std::printf("macro_world bake: %d^2 x 6 cells, %d erosion rounds, %.3f s\n",
                    cfg.macroWorld.resolution, cfg.macroWorld.erosionIterations, seconds);
        CHECK(seconds < 5.0);
        gpu.DestroyTexture(field);
    }
    pg_context_destroy(ctx);
}

TEST_CASE("macro_world cache: a recreated body reuses the field until a bake input changes", "[gpu][macro_world]")
{
    PgContext ctx = MakeHeadlessCtx();
    REQUIRE(ctx != nullptr);

    auto cfg = MacroEarth(64);
    auto generate = [&](const planetgen::BodyConfig& c)
    {
        // Same pattern as the app: a fresh body from the edited config's JSON.
        PgBody body = pg_body_create_from_json_string(ctx, planetgen::BodyConfigToJson(c).c_str());
        REQUIRE(body != nullptr);
        PgResult r = pg_generate_mesh(body, 8, 16, 42u);
        REQUIRE(r != nullptr);
        pg_result_destroy(r);
        pg_body_destroy(body);
        return ctx->macroBuiltKey;
    };

    const uint64_t first = generate(cfg);
    REQUIRE(first != 0);
    const auto texture = ctx->macroTexture;

    cfg.shape.heightScale *= 1.5f; // not read by the bake
    CHECK(generate(cfg) == first);
    CHECK(ctx->macroTexture == texture);

    cfg.macroWorld.erosionIterations += 1; // read by the bake
    CHECK(generate(cfg) != first);

    pg_context_destroy(ctx);
}
