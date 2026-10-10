// Water depth at an ocean vertex, in planet-radius units (> 0 under water, <= 0 on land).
// It is sea level minus the same geomorphed terrain height the planet pass draws for that
// vertex, so the ocean's coastline is the drawn coastline (mask, macro field and all).
float oceanDepth(float seaLevel, float height, float morphHeight, float morphFactor)
{
    return seaLevel - mix(height, morphHeight, morphFactor);
}
