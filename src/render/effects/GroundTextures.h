#pragma once

#include <cstdint>

namespace planets::render::effects
{

// The ground material texture arrays (detail + normal, one layer per GroundMaterial), generated
// from the body seed and uploaded with full mips. The ground shader reads them at fixed bindings.
class GroundTextures
{
public:
    static constexpr uint32_t DetailBinding = 6;
    static constexpr uint32_t NormalBinding = 7;

    GroundTextures() = default;
    ~GroundTextures();

    GroundTextures(const GroundTextures&) = delete;
    GroundTextures& operator=(const GroundTextures&) = delete;

    // Regenerates for this seed; a no-op when the seed is unchanged.
    void Generate(uint32_t seed);

    void Bind() const;

    bool IsReady() const { return _detail != 0; }

private:
    void Release();

    uint32_t _detail = 0;
    uint32_t _normal = 0;
    uint32_t _seed = 0;
};

} // namespace planets::render::effects
