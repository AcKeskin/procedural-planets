#pragma once

namespace planets::render
{

class BodyRuntime;

// Planet window > Terrain: shape, continents, mountains, plates, sea floor shape, terrain style
// and the erosion simulation. Everything here rebuilds the mesh, so it applies on release.
class TerrainTab
{
public:
    // Returns true when the terrain must regenerate.
    bool Draw(BodyRuntime& body);
};

} // namespace planets::render
