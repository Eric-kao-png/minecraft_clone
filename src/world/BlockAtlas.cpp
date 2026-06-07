#include "world/BlockAtlas.h"

#include "world/BlockId.h"

namespace BlockAtlas {

namespace {

constexpr int kGrassTop = 204;  // row 12, col 12
constexpr int kStone = 1;       // row 0, col 1
constexpr int kDirt = 2;        // row 0, col 2
constexpr int kGrassSide = 3;   // row 0, col 3
constexpr int kCobblestone = 16; // row 1, col 0

} // namespace

int atlasIndexForFace(uint8_t blockId, float normalX, float normalY, float normalZ) {
    (void)normalX;

    if (blockId == blocks::kGrass) {
        if (normalY > 0.5f) {
            return kGrassTop;
        }
        if (normalY < -0.5f) {
            return kDirt;
        }
        return kGrassSide;
    }

    switch (blockId) {
        case blocks::kStone:
            return kStone;
        case blocks::kDirt:
            return kDirt;
        case blocks::kCobblestone:
            return kCobblestone;
        default:
            return kStone;
    }
}

void tileOrigin(int atlasIndex, float& u0, float& v0) {
    const int col = atlasIndex % 16;
    const int row = atlasIndex / 16;
    u0 = static_cast<float>(col) * kTileSize;
    v0 = 1.0f - static_cast<float>(row + 1) * kTileSize;
}

} // namespace BlockAtlas
