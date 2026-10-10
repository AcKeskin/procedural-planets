
# Procedural Planets

A GPU-accelerated procedural planet renderer built with C++17 and OpenGL. Generates realistic celestial bodies — Earth-like planets, volcanic worlds, crystalline moons — with terrain, biomes, oceans, a scattering atmosphere and volumetric clouds, all driven by compute shaders and procedural noise. You can fly from orbit down to the ground without a loading step.

<p align="center">
  <img src="docs/images/hero_earth.png" alt="Earth-like procedural planet from space" width="720">
</p>

Inspired by [Sebastian Lague's Solar System](https://github.com/SebLague/Solar-System). The architecture and implementation are built from scratch in C++/OpenGL, but studying his approach helped shape the direction of this project.

> **A note on the rendering code:** I'm not a graphics developer — I understand the rendering pipeline at a high level but not the low-level details. The rendering parts (shaders, GPU compute, the OpenGL pipeline) were built with AI assistance (Claude). The overall architecture, the library design, and the rest of the codebase are my own.

## Gallery

<p align="center">
  <img src="docs/images/flyover.gif" alt="Orbit around the planet, then a descent through the clouds to a low view over the land" width="480">
</p>

An Earth-like world turning in real time:

https://github.com/user-attachments/assets/ebe561bd-8c4f-4574-84cc-a6a2f2054e71

_(Earth turntable video)_

Different body types generated back to back — Earth, volcanic, and crystalline worlds, fully procedural:



https://github.com/user-attachments/assets/895fe468-6541-4f7e-b797-4cf7dd774165


_(Planet-type showcase video)_

## Macro World Layer

An optional once-per-body bake on a coarse cube-sphere grid. It adds tectonic plates (mountain belts on convergent boundaries, rifts on divergent ones), hydraulic erosion, rivers that always drain to the sea, and moisture carried by prevailing winds (wet coasts, dry interiors, rain shadows). The terrain samples it as its base height and the climate model reads its moisture. The bake takes about 0.25 s on a desktop GPU.

Turn it on in the Terrain panel: **Generation > Macro World > Enable Macro World** (needs Use Tectonics). In a body JSON, set `"macroWorld": { "enabled": true }`. It is off by default. Its river carving still shows as stair-stepped streaks up close, which is why it is not on for Earth yet.

## Features

**Terrain and generation**
- **GPU Compute Terrain** — Height and shading generated in compute shader passes, entirely on the GPU
- **Async Generation** — Non-blocking terrain generation with a priority scheduler and GPU fence synchronization
- **Icosahedron LOD** — A quad-tree of spherical patches that split and merge with camera distance (max depth adjustable up to 12), with geomorphing so patches don't pop
- **Tectonic Plates** — Voronoi-based plate boundaries with convergent mountains and divergent rifts
- **Multi-Layer Noise** — Simplex and fractal noise with octaves, lacunarity, persistence, and ridge noise, plus fine relief that only shows up close
- **libplanetgen** — Standalone shared library (DLL) with C API for terrain generation, usable from Unity, Unreal, or any C/C++ consumer

**Ground**
- **Biomes** — Whittaker biome colours (tropical, temperate, boreal, tundra, ice) from temperature and moisture, read from a JSON palette per body
- **Generated Material Textures** — Rock, grass, sand, snow and soil textures generated from the planet seed at load (no image files), sampled triplanar at two scales so no tiling shows
- **Detail by Distance** — Colour and bump detail are added as you fly closer, based on how big a pixel is on the ground, so close-ups stay sharp and far views don't shimmer

**Water, sky and clouds**
- **Ocean** — Real water depth from the terrain, light absorption from shallow turquoise to deep blue, waves that keep their size at any planet radius, sun glitter, and a reflection of the actual sky
- **Atmosphere** — Rayleigh and Mie scattering with phase functions, an earth shadow, blue daytime sky, red sunsets and a real-size sun disc
- **Volumetric Clouds** — A raymarched cloud layer with a weather map, 3D noise, self-shadowing and a silver lining toward the sun; you can fly through it. The clouds cast shadows on the ground
- **HDR Rendering** — The scene renders in HDR with an exposure control, a tonemap that keeps highlight colour, and bloom around the sun and glints

**App**
- **Multi-Body Types** — Earth-like planets, volcanic worlds, crystalline moons — each with type-specific shaders, palettes, and GUI
- **Parameter Randomizer** — One-click randomization of all planet parameters within Earth-like constraints
- **Cinematic System** — Turntable camera (F5) and headless capture: single screenshots, turntable frame sequences, and a showcase that cycles through random worlds
- **Instrument Mode** — Script the camera and any setting from a JSON timeline (`--script`) or a live socket (`--live`), and capture frames; the README media is rendered this way
- **Live Tweaking** — Grouped ImGui panels with tooltips for every setting

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

The executable ends up in `build/bin/Release/`. The `planetgen.dll` shared library is built alongside, and `shaders/` and `data/` are copied next to the exe, so it runs from any folder.

Run the tests (they need a GPU; they use a hidden window) with:

```bash
ctest --test-dir build -C Release
```

## Dependencies

Everything is handled through CMake FetchContent, so you don't need to install anything manually.

| Library | Version | Purpose |
|---------|---------|---------|
| [GLFW](https://github.com/glfw/glfw) | 3.4 | Window and input |
| [GLM](https://github.com/g-truc/glm) | 1.0.1 | Mathematics |
| [ImGui](https://github.com/ocornut/imgui) | docking | Debug GUI |
| [gl3w](https://github.com/skaslev/gl3w) | latest | OpenGL loader |
| [stb](https://github.com/nothings/stb) | latest | Image loading and PNG export |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.11.3 | JSON parsing |
| [Catch2](https://github.com/catchorg/Catch2) | 3.6.0 | Tests |

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

All parameters are exposed in ImGui panels, grouped by topic with tooltips:

- **Scene** — sun direction, size and colour, stars, camera exposure and bloom, lighting
- **Terrain** — seed, height scale, continents, tectonics, the macro world layer, and the LOD quad-tree (mesh resolution, max depth, split threshold)
- **Surface** — adapts to the body type: Earth shows biome and climate controls, generic bodies show palette and noise parameters; also the ocean (colours, clarity, waves)
- **Atmosphere** — scattering colours and strength, haze (Mie), density, quality; the Earth Preset button restores the defaults
- **Clouds** — coverage, density, cloud size, base altitude, thickness, drift speed, shadow strength and quality. Clouds are part of the atmosphere, so they hide when the atmosphere is off

### Command line

Without arguments the app opens the interactive window. These flags run it headless or scripted:

| Flag | What it does |
|------|--------------|
| `--screenshot <path>` | Render a few warm-up frames, write one PNG, exit. Takes `--seed`, `--width`, `--height`, `--camera x,y,z`, `--light x,y,z`, `--frames` |
| `--cinematic --out-pattern <out/f_%04d.png>` | Render a turntable as a PNG sequence (`--cinematic-frames`, `--fps`, `--orbit-speed`) |
| `--showcase --out-pattern <...>` | Orbit while generating a new random world every few seconds (`--planets`, `--hold-frames`) |
| `--script <timeline.json>` | Run a JSON timeline of camera moves, setting changes and captures (samples in `data/scripts/`) |
| `--live [port]` | Open the window and accept the same JSON commands over a local socket (default port 8787) |

## Project Structure

```
src/                    The showcase app — frame loop, LOD, rendering, GUI
├── core/               Platform-independent logic
│   ├── math/           Camera system
│   └── noise/          Simplex and fractal noise
├── render/             GPU rendering pipeline
│   ├── lod/            Icosahedron patch quad-tree (PlanetQuadTree, SpherePatch, PatchPool)
│   ├── effects/        Atmosphere, clouds, ocean, bloom and ground texture passes
│   ├── settings/       Typed configuration structs
│   ├── gui/            ImGui panels that edit the active body's config
│   ├── cinematic/      Turntable controller + screenshot capture
│   ├── Renderer             Multi-pass HDR scene draw (space → planet → ocean → clouds + atmosphere → tonemap)
│   ├── BodyRuntime          Render-time view of the active body (no subclassing)
│   └── GenerationScheduler  Async compute dispatch with GPU fence sync
└── app/                Application entry, input handling, headless capture
    └── instrument/     --script / --live command timelines and parameter registry

libplanetgen/           Standalone terrain generation library (DLL)
├── include/planetgen/  Public C API + C++ RAII wrapper
├── src/api/            C API implementation
├── src/model/          BodyConfig (canonical typed body) + JSON + palette registry
├── src/strategy/       GenerationPipeline + Height/Erosion/Shading/Palette strategies, macro world bake
├── src/materials/      CPU generators for the ground textures and the cloud noise
├── src/backend/        Compute backend abstraction
│   └── opengl/         OpenGL 4.5 compute implementation
└── cmake/              Shader embedding build tools

shaders/
├── compute/            GPU terrain generation (height, shading, erosion)
├── includes/           Shared GLSL libraries (noise, lighting, sky scattering, cloud layer, ground materials, tonemap)
├── earth/              Earth-specific surface rendering
├── generic/            Palette-based surface rendering
├── atmosphere.*        Atmospheric scattering and the final composite
├── clouds_march.frag   Volumetric cloud raymarch
├── ocean.*             Ocean surface
└── space.*             Background rendering

data/
├── bodies/             Body type configs (earth.json, volcanic.json, crystalline.json)
├── palettes/           Color palettes per body type (earth.json, volcanic.json, etc.)
└── scripts/            Sample instrument timelines for --script
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
6. **Surface Rendering** — Body-type-specific fragment shaders apply biome colours and generated material textures (Earth) or palette gradients (generic), with lighting and cloud shadows
7. **Ocean** — Drawn on the terrain's own patches at sea level, coloured by real water depth and reflecting the sky
8. **Clouds and Atmosphere** — The cloud layer is raymarched at half resolution, then the atmosphere pass composites it with the scene and adds scattering along each view ray. Both use the same sky model (`sky_radiance.glsl`), as does the ocean reflection
9. **Tonemap** — The HDR image gets exposure, bloom and a tonemap before the UI is drawn

## References

- [Sebastian Lague — Solar System](https://github.com/SebLague/Solar-System) — studied for compute-based terrain generation approach
- [Jujedie — PlanetGenerator](https://github.com/Jujedie/PlanetGenerator) — the macro world's erosion passes are ported from it (MIT, see `third_party/NOTICE-PlanetGenerator.md`)
- [Inigo Quilez — Smooth Min/Max](https://iquilezles.org/articles/smin/) — polynomial smooth blending functions
- [Ben Golus — Triplanar Shader](https://bgolus.medium.com/normal-mapping-for-a-triplanar-shader-10bf39dca05a) — triplanar mapping with reoriented normal blending
- [Andrew Schneider — The Real-time Volumetric Cloudscapes of Horizon Zero Dawn](https://www.guerrilla-games.com/read/the-real-time-volumetric-cloudscapes-of-horizon-zero-dawn) — weather map, Perlin-Worley noise and the powder term for the clouds
- [Sébastien Hillaire — Physically Based Sky, Atmosphere and Cloud Rendering in Frostbite](https://www.ea.com/frostbite/news/physically-based-sky-atmosphere-and-cloud-rendering) — energy-conserving scattering integration and multiple-scattering approximation

## License

[MIT](LICENSE)
