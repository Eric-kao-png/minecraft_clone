#pragma once

#include <cstdint>

namespace BlockAtlas {

constexpr float kTileSize = 1.0f / 16.0f;

int atlasIndexForFace(uint8_t blockId, float normalX, float normalY, float normalZ);

void tileOrigin(int atlasIndex, float& u0, float& v0);

} // namespace BlockAtlas
