#include "ContinentMaskRenderer.h"
#define NOMINMAX
#include <GL/gl3w.h>
#include <iostream>
#include <algorithm>

namespace planets::render
{

ContinentMaskRenderer::ContinentMaskRenderer() = default;

ContinentMaskRenderer::~ContinentMaskRenderer()
{
    DestroyTexture();
}

bool ContinentMaskRenderer::Initialize()
{
    if (_shader.IsValid())
        return true; // already loaded

    if (!_shader.LoadFromFile("shaders/compute/continent_growth.comp"))
    {
        std::cerr << "[ContinentMaskRenderer] Failed to load continent_growth.comp" << std::endl;
        return false;
    }
    return true;
}

void ContinentMaskRenderer::Generate(const planetgen::BodyConfig& config, uint32_t seed)
{
    if (!_shader.IsValid())
    {
        std::cerr << "[ContinentMaskRenderer] Shader not initialised — call Initialize() first" << std::endl;
        return;
    }

    const auto& sh = config.shape;
    uint32_t res = static_cast<uint32_t>(
        (std::max)(8, (std::min)(sh.continentMaskResolution, 256)));

    // Recreate texture at new resolution if needed
    DestroyTexture();
    glGenTextures(1, &_texture);
    glBindTexture(GL_TEXTURE_3D, _texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RG32F,
                 static_cast<GLsizei>(res), static_cast<GLsizei>(res), static_cast<GLsizei>(res),
                 0, GL_RG, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_3D, 0);

    // Bind as image3D for compute write (binding = 0, as declared in shader)
    glBindImageTexture(0, _texture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RG32F);

    // Create and upload the UBO matching ContinentGrowthParams (binding = 1)
    struct alignas(16) Params
    {
        uint32_t count;
        uint32_t seed;
        uint32_t resolution;
        uint32_t iterations;
        float    sizeVariance;
        float    clustering;
        float    _pad0;
        float    _pad1;
    };

    Params p{};
    p.count       = static_cast<uint32_t>((std::max)(1, sh.continentCount));
    p.seed        = seed;
    p.resolution  = res;
    p.iterations  = 3u;  // warp octaves for coastline detail
    p.sizeVariance = sh.continentSizeVariance;
    p.clustering  = sh.continentClustering;

    GLuint ubo = 0;
    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(p), &p, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Dispatch — local_size = 8³, so ceil(res/8) groups per axis
    _shader.Use();
    uint32_t groups = (res + 7u) / 8u;
    _shader.Dispatch(groups, groups, groups);

    // Barrier before the height shader samples this texture
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    glDeleteBuffers(1, &ubo);

    std::cout << "[ContinentMaskRenderer] Generated " << res << "³ mask (count="
              << p.count << " seed=" << seed << ")" << std::endl;
}

void ContinentMaskRenderer::DestroyTexture()
{
    if (_texture)
    {
        glDeleteTextures(1, &_texture);
        _texture = 0;
    }
}

} // namespace planets::render
