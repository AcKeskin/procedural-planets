#include "CelestialBody.h"

namespace planets::render
{

void CelestialBody::SetRadius(float radius)
{
    _radius = radius;
}

void CelestialBody::SetSeaLevel(float seaLevel)
{
    _seaLevel = seaLevel;
}

void CelestialBody::SetAtmosphereHeight(float height)
{
    _atmosphereHeight = height;
}

std::string CelestialBody::GetErosionShaderPath() const
{
    return "";
}

} // namespace planets::render
