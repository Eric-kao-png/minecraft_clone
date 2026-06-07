#pragma once

#include <cstdint>

namespace GameConfig {

constexpr unsigned int kScreenWidth = 1280;
constexpr unsigned int kScreenHeight = 720;

constexpr int kLoadRadius = 4;
constexpr int kUnloadRadius = 6;

constexpr const char* kWindowTitle = "Voxel Chunks";
constexpr const char* kShaderVertexPath = "../shaders/voxel_lit.vs";
constexpr const char* kShaderFragmentPath = "../shaders/voxel_lit.fs";
constexpr const char* kAtlasTexturePath = "../resources/minecraft_atlas.png";

constexpr uint32_t kWorldSeed = 2026;

} // namespace GameConfig
