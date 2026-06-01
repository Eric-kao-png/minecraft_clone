# A C++/OpenGL Minecraft Clone

![C++](https://img.shields.io/badge/Language-C%2B%2B-blue.svg)
![OpenGL](https://img.shields.io/badge/Graphics-OpenGL%203.3+-green.svg)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)

`minecraft_clone` is a high-performance voxel game engine built from scratch using C++ and OpenGL 3.3+. This project implements the core architecture of a voxel-based world, featuring a dynamic chunk system, procedural terrain generation, physical interactions, and custom lighting shaders.



## Core Features

* **Efficient World Management**:
    * **Chunk-based Architecture**: Divides the world into $16 \times 16 \times 16$ manageable sections to optimize rendering and memory usage.
    * **Advanced World Meshing**: Implements Face Culling to significantly reduce the number of vertices sent to the GPU by rendering only visible faces.
* **Dynamic Lighting System**:
    * Implements the Phong Lighting Model including Ambient, Diffuse, and Specular components.
    * Utilizes GLSL shaders (`voxel_lit.vs/fs`) for lit voxel rendering.
* **Physics & Interaction**:
    * **AABB Collision Detection**: Precise interaction between the player and the voxel world, preventing clipping and enabling gravity.
    * **World Raycasting**: Accurately determines which block the player is looking at, providing the foundation for block placement and destruction.
* **Procedural Generation**:
    * Dynamic terrain generation logic that creates varied landscapes.



## Technical Stack

* **Language**: C++17
* **Graphics API**: OpenGL 3.3 Core Profile
* **Libraries**:
    * **GLFW**: Window management and input handling.
    * **Glad**: OpenGL function loader.
    * **GLM (OpenGL Mathematics)**: Vector and matrix mathematics.
    * **stb_image**: Image loading for textures.
* **Build System**: CMake

## Project Structure

```text
├── include/
│   ├── app/            # Application entry, GameConfig
│   ├── game/           # Input handling
│   ├── render/         # Lighting, texture loading
│   ├── world/          # Chunks, meshing, terrain, block IDs
│   └── physics/        # Player, collision
├── src/                # Implementations matching include/
├── engine/             # Camera & Shader wrappers
├── external/           # glad, glm, stb (third-party)
├── shaders/            # GLSL (e.g. voxel_lit.vs/fs)
└── resources/          # Texture atlases and image assets
```
**Getting Started**:
* **Prerequisites**:
  * A graphics driver with OpenGL 3.3+ support.
  * CMake 3.10 or higher.
  * C++ compiler (GCC/Clang or MSVC).

Build Instructions:

Clone the repository:

```Bash
git clone [https://github.com/Eric-kao-png/minecraft_clone.git](https://github.com/Eric-kao-png/minecraft_clone.git)
cd minecraft_clone
```
Generate build files:

```Bash
mkdir build && cd build
cmake ..
```
Compile and run (from project root):

```bash
cmake -B build -S . -DCMAKE_PREFIX_PATH="$(brew --prefix glfw)"
cmake --build build
cd build && ./minecraft_clone
```

### Cursor / VS Code

Open this folder in Cursor.

- **Build**: `Cmd+Shift+B` (default task: Build minecraft_clone)
- **Run game** (no debugger): `Cmd+Shift+P` → **Tasks: Run Task** → **Run minecraft_clone (game)**
- **Run with debugger**: install extension **CodeLLDB**, then Run and Debug → **Run minecraft_clone** → F5

Roadmap:
  * Implement Greedy Meshing for further performance optimization.

  * Add support for transparent blocks (glass) and fluid rendering (water).

  * Integrate Perlin Noise for more realistic biomes.

  * Multi-threaded chunk loading.

License:
  * Distributed under the MIT License. See LICENSE for more information.