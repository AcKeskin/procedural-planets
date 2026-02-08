# Procedural Planets

A GPU-accelerated procedural planet renderer built with C++17 and OpenGL. Generates realistic, Earth-like planets with terrain, atmosphere, oceans, and biomes — all driven by compute shaders and procedural noise.

<p align="center">
  <img src="docs/videos/spedup_cinematic_updated.gif" alt="Procedural planet cinematic" width="600">
</p>

Inspired by [Sebastian Lague's Solar System](https://github.com/SebLague/Solar-System). The architecture and implementation are built from scratch in C++/OpenGL, but studying his approach helped shape the direction of this project.

## Features

- **GPU Compute Terrain** — Height and shading generated in two compute shader passes, entirely on the GPU
- **Icosahedron LOD** — Subdivided into patches with 4 detail levels, switching based on camera distance
- **Atmospheric Scattering** — Rayleigh scattering with angle-dependent glow
- **Ocean Rendering** — Separate pass with configurable sea level
- **Biome System** — Classification by temperature, moisture, and height with triplanar texturing
- **Multi-Layer Noise** — Simplex and fractal noise with octaves, lacunarity, persistence, and ridge noise
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
│   ├── settings/       Typed configuration (terrain, ocean, surface, scene)
│   └── gui/            ImGui debug panels
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
2. **Height Pass** — A compute shader generates terrain elevation from layered noise (continents, mountains, ocean masks)
3. **Shading Pass** — Another compute shader classifies biomes and surface detail from height, temperature, and moisture
4. **LOD Selection** — Each frame, patches pick their detail level based on distance and get culled if outside the view
5. **Surface Rendering** — The displaced mesh is rendered with triplanar texturing and biome-driven coloring
6. **Effects** — Atmospheric scattering and ocean are composited as separate passes on top

## License

TBD
