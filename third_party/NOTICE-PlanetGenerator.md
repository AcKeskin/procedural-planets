# Jujedie/PlanetGenerator

The macro world erosion passes are ported from Jujedie/PlanetGenerator
(https://github.com/Jujedie/PlanetGenerator), MIT licence (text below).

| Our file | Upstream file | Upstream commit |
|----------|---------------|-----------------|
| `shaders/compute/macro_erosion_rainfall.comp` | `shader/compute/erosion/erosion_rainfall.glsl` | 76c1513c49539716f541dac67294ce29479b57de |
| `shaders/compute/macro_erosion_flow.comp` | `shader/compute/erosion/erosion_flow.glsl` | 76c1513c49539716f541dac67294ce29479b57de |
| `shaders/compute/macro_erosion_flux.comp` | `shader/compute/erosion/erosion_flux_accumulation.glsl` | 76c1513c49539716f541dac67294ce29479b57de |
| `shaders/compute/macro_erosion_sediment.comp` | `shader/compute/erosion/erosion_sediment.glsl` | 18c1988 (upstream later disabled incision; this is the last version that erodes) |

Changes made in the port:

- Equirectangular image grid replaced by the cube-sphere grid
  (`shaders/includes/macro_grid.glsl`), including cross-face neighbours.
- Cells are near-equal-area, so the cos(latitude) metric is dropped.
- Parameters come from one shared std140 block (`shaders/includes/macro_params.glsl`).
- Rainfall uses uniform precipitation and a latitude + elevation temperature.
- The flow pass also carries suspended sediment with the water (upstream keeps it
  in place), so coarse cells can carve valleys; ocean cells keep what arrives.
- Flux accumulation compares the neighbour's drain target cell instead of an
  opposite direction index, and propagates without decay.
- Per-pass erosion cap is 12 m (the constant from upstream commit eef8912) instead
  of the resolution-scaled cap, so valleys show at the coarse macro cell size.

## Licence

```
MIT License

Copyright (c) 2025 Jujedie

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
