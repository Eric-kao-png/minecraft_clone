# minecraft_clone

![C++](https://img.shields.io/badge/Language-C%2B%2B-blue.svg)
![OpenGL](https://img.shields.io/badge/Graphics-OpenGL%203.3+-green.svg)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)

A voxel sandbox prototype built with **C++17** and **OpenGL 3.3 Core**. The project features chunk-based world streaming, procedural terrain, Phong lighting with a day/night cycle, player physics, and block placement/destruction.

## Core Features

- **Chunk world**
  - Chunks are **16 × 96 × 16** blocks
  - Dynamic load/unload around the player
  - Face culling meshing (only visible block faces are rendered)
- **Lighting**
  - Phong model (ambient, diffuse, specular)
  - Orbiting sun and moon directional lights
  - Player flashlight (spot light)
  - Shaders: `shaders/voxel_lit.vs` / `shaders/voxel_lit.fs`
- **Physics & interaction**
  - AABB collision and gravity
  - World raycasting for block targeting
  - Break (left click) and place (right click) blocks
- **Terrain**
  - Procedural heightmap generation (FBM noise)
  - Block types: stone, dirt, grass, cobblestone

## Controls

| Input | Action |
|-------|--------|
| W / A / S / D | Move |
| Space | Jump |
| Left Shift | Sprint |
| Mouse | Look |
| Scroll wheel | Adjust FOV |
| Left click | Break block |
| Right click | Place block |
| 1 | Select dirt |
| 2 | Select grass |
| 3 | Select cobblestone |
| 4 | Select stone |
| Tab | Release / capture mouse cursor |
| Esc | Quit |

## Technical Stack

| Component | Choice |
|-----------|--------|
| Language | C++17 |
| Graphics | OpenGL 3.3 Core |
| Window / input | GLFW |
| Loader | Glad |
| Math | GLM |
| Textures | stb_image |
| Build | CMake ≥ 3.10 |

## Project Structure

```text
├── include/
│   ├── app/            # Application, GameConfig
│   ├── game/           # Input handling
│   ├── render/         # Lighting, texture loading
│   ├── world/          # Chunks, meshing, terrain, atlas
│   └── physics/        # Player, collision
├── src/                # Implementations
├── engine/             # Camera, Shader helpers
├── external/           # glad, glm, stb
├── shaders/            # GLSL (voxel_lit.vs/fs)
└── resources/          # minecraft_atlas.png (+ optional standalone textures)
```

Runtime block rendering uses **`resources/minecraft_atlas.png`** (16×16 tile grid). Grass blocks use multiple atlas tiles (top / side / bottom).

## Getting Started

### Prerequisites

- OpenGL 3.3+ capable GPU and drivers
- CMake 3.10+
- C++17 compiler (Clang, GCC, or MSVC)
- **GLFW 3** (on macOS: `brew install glfw`)

### Build & run

From the project root:

```bash
cmake -B build -S . -DCMAKE_PREFIX_PATH="$(brew --prefix glfw)"
cmake --build build
cd build && ./minecraft_clone
```

> **Important:** Run the executable from the `build/` directory (or use the VS Code run task below). Shader and texture paths are relative to that working directory.

After the first configure, incremental builds are enough:

```bash
cmake --build build
cd build && ./minecraft_clone
```

### Cursor / VS Code

Open this folder in Cursor or VS Code.

- **Build:** `Cmd+Shift+B` → **Build minecraft_clone**
- **Run:** `Cmd+Shift+P` → **Tasks: Run Task** → **Run minecraft_clone (game)**
- **Debug:** Install **CodeLLDB**, then Run and Debug → **Run minecraft_clone** → F5

Recommended extensions are listed in `.vscode/extensions.json`.

## Roadmap

- Greedy meshing for fewer vertices
- Transparent blocks (glass) and fluids (water)
- Richer biomes (e.g. Perlin noise)
- Multi-threaded chunk loading

## Acknowledgments

Block textures in this project were created with help from the following open-source tools and models:

- **[FLUX.1 [schnell]](https://huggingface.co/black-forest-labs/FLUX.1-schnell)** by [Black Forest Labs](https://huggingface.co/black-forest-labs) — text-to-image generation for texture concepts and reference imagery (Apache-2.0).
- **[Aseprite](https://www.aseprite.org/)** — pixel-art editing for refining block textures.

## License

MIT License.
