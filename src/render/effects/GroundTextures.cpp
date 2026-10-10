#include "GroundTextures.h"

#include <GL/gl3w.h>
#include <materials/GroundMaterials.h>
#include <chrono>
#include <cmath>
#include <iostream>

namespace planets::render::effects
{

namespace
{
constexpr int TextureSize = 512;
constexpr GLenum MaxAnisotropy = 0x84FE; // GL_TEXTURE_MAX_ANISOTROPY (core in 4.6, ARB/EXT in 4.5)
constexpr float Anisotropy = 8.0f;

uint32_t CreateArray(const std::vector<std::vector<uint8_t>>& layers, int size)
{
    const int levels = 1 + static_cast<int>(std::floor(std::log2(static_cast<float>(size))));
    uint32_t texture = 0;
    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &texture);
    glTextureStorage3D(texture, levels, GL_RGBA8, size, size, static_cast<GLsizei>(layers.size()));
    for (size_t layer = 0; layer < layers.size(); ++layer)
    {
        glTextureSubImage3D(texture, 0, 0, 0, static_cast<GLint>(layer), size, size, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                            layers[layer].data());
    }
    glGenerateTextureMipmap(texture);
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameterf(texture, MaxAnisotropy, Anisotropy);
    return texture;
}
} // namespace

GroundTextures::~GroundTextures()
{
    Release();
}

void GroundTextures::Generate(uint32_t seed)
{
    if (IsReady() && seed == _seed)
        return;
    Release();

    const auto start = std::chrono::steady_clock::now();
    planetgen::GroundMaterialSet set = planetgen::GenerateGroundMaterials(seed, TextureSize);
    _detail = CreateArray(set.detail, set.size);
    _normal = CreateArray(set.normal, set.size);
    _seed = seed;
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::cout << "[GroundTextures] Generated " << planetgen::GroundMaterialCount << " materials at " << TextureSize
              << "^2 in " << ms << " ms" << std::endl;
}

void GroundTextures::Bind() const
{
    glBindTextureUnit(DetailBinding, _detail);
    glBindTextureUnit(NormalBinding, _normal);
}

void GroundTextures::Release()
{
    if (_detail)
        glDeleteTextures(1, &_detail);
    if (_normal)
        glDeleteTextures(1, &_normal);
    _detail = 0;
    _normal = 0;
}

} // namespace planets::render::effects
