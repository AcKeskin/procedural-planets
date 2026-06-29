# Procedural Planets

A GPU-accelerated procedural planet renderer built with C++17 and OpenGL. Generates realistic celestial bodies — Earth-like planets, volcanic worlds, crystalline moons — with terrain, atmosphere, oceans, and biomes, all driven by compute shaders and procedural noise.

<p align="center">
  <img src="docs/videos/spedup_cinematic_updated.gif" alt="Procedural planet cinematic" width="600">
</p>

Inspired by [Sebastian Lague's Solar System](https://github.com/SebLague/Solar-System). The architecture and implementation are built from scratch in C++/OpenGL, but studying his approach helped shape the direction of this project.

## Features

- **GPU Compute Terrain** — Height and shading generated in two compute shader passes, entirely on the GPU
- **Async Generation** — Non-blocking terrain generation with a priority scheduler and GPU fence synchronization
- **Icosahedron LOD** — Subdivided into patches with 4 detail levels, switching based on camera distance
- **Multi-Body Types** — Earth-like planets, volcanic worlds, crystalline moons — each with type-specific shaders, palettes, and GUI
- **libplanetgen** — Standalone shared library (DLL) with C API for terrain generation, usable from Unity, Unreal, or any C/C++ consumer
- **Climate Model** — Whittaker biome classification from latitude, elevation, and Hadley circulation for Earth-like bodies
- **Data-Driven Palettes** — JSON-defined color palettes per body type, blended by height in the fragment shader
- **Atmospheric Scattering** — Rayleigh scattering with wavelength-dependent coloring and angle-based glow
- **Ocean Rendering** — Depth-based shallow-to-deep color blending, animated procedural wave normals, and specular highlights
- **Tectonic Plates** — Voronoi-based plate boundaries with convergent mountains and divergent rifts
- **Multi-Layer Noise** — Simplex and fractal noise with octaves, lacunarity, persistence, and ridge noise
- **Parameter Randomizer** — One-click randomization of all planet parameters within Earth-like constraints
- **Cinematic System** — Turntable camera with keyframe animation and screenshot/GIF capture
- **Live Tweaking** — Body-type-aware ImGui panels for all parameters

<p align="center">
  <img src="docs/images/with_atmosphere_0.png" alt="Planet with atmospheric scattering" width="400">
  <img src="docs/images/without_atmosphere_0.png" alt="Planet terrain without atmosphere" width="400">
</p>

## Building

You'll need CMake 3.20+, a C++17 compiler, and OpenGL 4.5+ support. All dependencies are pulled in automatically — no manual setup needed.

```bash
git clone https://github.com/AcKeskin/procedural-planets.git
cd procedural-planets
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The executable ends up in `build/bin/Release/`. The `planetgen.dll` shared library is built alongside.

## Dependencies

Everything is handled through CMake FetchContent, so you don't need to install anything manually.

| Library | Version | Purpose |
|---------|---------|---------|
| [GLFW](https://github.com/glfw/glfw) | 3.4 | Window and input |
| [GLM](https://github.com/g-truc/glm) | 1.0.1 | Mathematics |
| [ImGui](https://github.com/ocornut/imgui) | docking | Debug GUI |
| [gl3w](https://github.com/skaslev/gl3w) | latest | OpenGL loader |
| [stb](https://github.com/nothings/stb) | latest | Image loading |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.11.3 | JSON parsing |

## Controls

| Input | Action |
|-------|--------|
| `W A S D` | Move camera |
| `E / Q` | Move up / down |
| `Right Mouse` + drag | Look around |
| `Shift` | Speed boost (5x) |
| `G` | Toggle orbit / free-fly camera |
| `H` | Toggle atmosphere |
| `R` | Randomize planet |
| `Tab` | Toggle GUI visibility |
| `F5` | Play / stop cinematic turntable |
| `F12` | Take screenshot (PNG) |
| `Esc` | Stop cinematic / quit |

All terrain, atmosphere, ocean, and scene parameters are exposed in ImGui panels. The Surface panel adapts to the active body type — Earth shows biome/climate controls, generic bodies show palette and noise parameters.

## Project Structure

```
src/                    The showcase app — frame loop, LOD, rendering, GUI
├── core/               Platform-independent logic
│   ├── math/           Camera system
│   └── noise/          Simplex and fractal noise
├── render/             GPU rendering pipeline
│   ├── lod/            Icosahedron patch quad-tree (PlanetQuadTree, SpherePatch, PatchPool)
│   ├── effects/        Atmosphere and ocean renderers
│   ├── settings/       Typed configuration structs
│   ├── gui/            ImGui panels that edit the active body's config
│   ├── cinematic/      Turntable controller + screenshot capture
│   ├── Renderer             Multi-pass scene draw (space → planet → ocean → atmosphere)
│   ├── BodyRuntime          Render-time view of the active body (no subclassing)
│   └── GenerationScheduler  Async compute dispatch with GPU fence sync
└── app/                Application entry, input handling, headless capture

libplanetgen/           Standalone terrain generation library (DLL)
├── include/planetgen/  Public C API + C++ RAII wrapper
├── src/api/            C API implementation
├── src/model/          BodyConfig (canonical typed body) + JSON + palette registry
├── src/strategy/       GenerationPipeline + Height/Erosion/Shading/Palette strategies
├── src/backend/        Compute backend abstraction
│   └── opengl/         OpenGL 4.5 compute implementation
└── cmake/              Shader embedding build tools

shaders/
├── compute/            GPU terrain generation (height, shading, erosion)
├── includes/           Shared GLSL libraries (noise, math, lighting)
├── earth/              Earth-specific surface rendering
├── generic/            Palette-based surface rendering
├── atmosphere.*        Atmospheric scattering
├── ocean.*             Ocean surface
└── space.*             Background rendering

data/
├── bodies/             Body type configs (earth.json, volcanic.json, crystalline.json)
└── palettes/           Color palettes per body type (earth.json, volcanic.json, etc.)
```

## Architecture

### Body Type System

A body is **data, not a subclass**. The canonical definition is `BodyConfig` (in `libplanetgen/src/model/`) — a typed struct of blocks (shape, tectonics, ocean floor, shading, palette, shader paths) loaded from `data/bodies/*.json`. It is the single source of truth for what a body is.

The app wraps the active config in a `BodyRuntime` — a render-time *adapter* (no inheritance) that exposes metadata, shader paths, and render-uniform binding. The ImGui panels edit the config in place; any edit rebuilds the body so its continent mask re-bakes.

New body types are added by creating a JSON config + palette file. No code changes required.

### libplanetgen

A standalone shared library that owns the GPU compute terrain pipeline as a reusable DLL:
- **C API** (`planetgen.h`) — opaque handles for context, body, and result; designed for C# P/Invoke and FFI
- **Strategy pipeline** — `GenerationPipeline` runs fixed per-stage strategies (Height → Erosion → Shading → Palette); each stage is an interface, so a new stage is a new class, not a fork
- **Embedded shaders** — compute shaders baked into the DLL at build time, no runtime file dependencies
- **Backend abstraction** — `IComputeBackend` interface allows future Vulkan support without API changes
- **Dual context mode** — reuses host app's GL context or creates a headless GLFW window

The app drives terrain through the library's **per-patch** entry point (`pg_generate_patch`): the same per-vertex strategies as the whole-mesh path, writing GPU-resident into the app's own LOD buffers, with no erosion (it is neighbour-coupled and would seam at patch boundaries). The app's `GenerationScheduler` owns the async dispatch and GPU fence sync.

### Generation Pipeline

1. **Patch Generation** — An icosahedron is subdivided into spherical patches projected onto a unit sphere
2. **Async Scheduling** — Patches are queued into a priority scheduler that dispatches compute work across frames with GPU fence synchronization
3. **Height Pass** — A compute shader generates terrain elevation from layered noise (continents, mountains, tectonic plates, ocean floor topology)
4. **Shading Pass** — Another compute shader computes per-vertex climate data (temperature, moisture) or palette parameters
5. **LOD Selection** — Each frame, patches pick their detail level based on distance and get culled if outside the view
6. **Surface Rendering** — Body-type-specific fragment shaders apply biome colors (Earth) or palette gradients (generic) with lighting, fresnel, and haze
7. **Effects** — Atmospheric scattering and ocean with depth coloring and wave animation are composited as post-processing passes

## References

- [Sebastian Lague — Solar System](https://github.com/SebLague/Solar-System) — studied for compute-based terrain generation approach
- [Inigo Quilez — Smooth Min/Max](https://iquilezles.org/articles/smin/) — polynomial smooth blending functions
- [Ben Golus — Triplanar Shader](https://bgolus.medium.com/normal-mapping-for-a-triplanar-shader-10bf39dca05a) — triplanar mapping with reoriented normal blending

## License

[MIT](LICENSE)
