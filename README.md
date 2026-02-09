# Procedural Planets

A GPU-accelerated procedural planet renderer built with C++17 and OpenGL. Generates realistic, Earth-like planets with terrain, atmosphere, oceans, and biomes — all driven by compute shaders and procedural noise.

<p align="center">
  <img src="docs/videos/spedup_cinematic_updated.gif" alt="Procedural planet cinematic" width="600">
</p>

Inspired by [Sebastian Lague's Solar System](https://github.com/SebLague/Solar-System). The architecture and implementation are built from scratch in C++/OpenGL, but studying his approach helped shape the direction of this project.

## Features

- **GPU Compute Terrain** — Height and shading generated in two compute shader passes, entirely on the GPU
- **Async Generation** — Non-blocking terrain generation with a priority scheduler and GPU fence synchronization
- **Icosahedron LOD** — Subdivided into patches with 4 detail levels, switching based on camera distance
- **Atmospheric Scattering** — Rayleigh scattering with wavelength-dependent coloring and angle-based glow
- **Ocean Rendering** — Depth-based shallow-to-deep color blending, animated procedural wave normals, and specular highlights
- **Fresnel Rim** — Distance-adaptive rim lighting that scales with camera distance for a sense of planetary scale
- **Biome System** — Classification by temperature, moisture, and height with triplanar texturing
- **Multi-Layer Noise** — Simplex and fractal noise with octaves, lacunarity, persistence, and ridge noise
- **Parameter Randomizer** — One-click randomization of all planet parameters within Earth-like constraints
- **Camera Modes** — FreeFly and Orbit cameras
- **Live Tweaking** — ImGui debug panels for all parameters

<p align="center">
  <img src="docs/images/with_atmosphere_0.png" alt="Planet with atmospheric scattering" width="400">
  <img src="docs/images/without_atmosphere_0.png" alt="Planet terrain without atmosphere" width="400">
</p>

## Building

You'll need CMake 3.20+, a C++17 compiler, and OpenGL 4.3+ support. All dependencies are pulled in automatically — no manual setup needed.

```bash
git clone https://github.com/AcKeskin/procedural-planets.git
cd procedural-planets
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The executable ends up in `build/bin/Release/`.

## Dependencies

Everything is handled through CMake FetchContent, so you don't need to install anything manually.

| Library | Version | Purpose |
|---------|---------|---------|
| [GLFW](https://github.com/glfw/glfw) | 3.4 | Window and input |
| [GLM](https://github.com/g-truc/glm) | 1.0.1 | Mathematics |
| [ImGui](https://github.com/ocornut/imgui) | docking | Debug GUI |
| [gl3w](https://github.com/skaslev/gl3w) | latest | OpenGL loader |
| [stb](https://github.com/nothings/stb) | latest | Image loading |

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

All terrain, atmosphere, ocean, and scene parameters are exposed in ImGui panels on the left side of the viewport. Screenshots are saved to the `captures/` directory.

## Project Structure

```
src/
├── core/               Platform-independent logic
│   ├── math/           Camera system
│   ├── noise/          Simplex and fractal noise
│   └── generation/     Planet model, noise layers, terrain generation
├── render/             GPU rendering pipeline
│   ├── lod/            Icosahedron patch LOD system
│   ├── effects/        Atmosphere and ocean renderers
│   ├── settings/       Typed configuration (terrain, ocean, surface, scene, atmosphere)
│   ├── gui/            ImGui debug panels
│   ├── GenerationScheduler   Async compute dispatch with GPU fence sync
│   └── ParameterRandomizer   Constrained Earth-like parameter generation
└── app/                Application entry, input handling

shaders/
├── compute/            GPU terrain generation (height + shading passes)
├── includes/           Shared GLSL libraries (noise, math, triplanar)
├── planet.*            Planet surface rendering
├── atmosphere.*        Atmospheric scattering
├── ocean.*             Ocean surface
└── space.*             Background rendering
```

## How It Works

1. **Patch Generation** — An icosahedron is subdivided into spherical patches projected onto a unit sphere
2. **Async Scheduling** — Patches are queued into a priority scheduler that dispatches compute work across frames with GPU fence synchronization, keeping the UI responsive
3. **Height Pass** — A compute shader generates terrain elevation from layered noise (continents, mountains, ocean masks)
4. **Shading Pass** — Another compute shader classifies biomes and surface detail from height, temperature, and moisture
5. **LOD Selection** — Each frame, patches pick their detail level based on distance and get culled if outside the view
6. **Surface Rendering** — The displaced mesh is rendered with triplanar texturing, biome-driven coloring, and distance-adaptive fresnel rim
7. **Effects** — Atmospheric scattering and ocean with depth coloring and wave animation are composited as post-processing passes

## References

- [Sebastian Lague — Solar System](https://github.com/SebLague/Solar-System) — studied for compute-based terrain generation approach
- [Inigo Quilez — Smooth Min/Max](https://iquilezles.org/articles/smin/) — polynomial smooth blending functions
- [Ben Golus — Triplanar Shader](https://bgolus.medium.com/normal-mapping-for-a-triplanar-shader-10bf39dca05a) — triplanar mapping with reoriented normal blending

## License

[MIT](LICENSE)
