#pragma once

namespace planets::core
{

// Interface for all noise generators
class INoiseGenerator
{
public:
    virtual ~INoiseGenerator() = default;

    // Sample noise at 3D position, returns value in [0, 1]
    virtual float Sample(float x, float y, float z) const = 0;
};

} // namespace planets::core
