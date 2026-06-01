#include "world/BlockAtlas.h"

#include "world/BlockId.h"

namespace BlockAtlas {

FaceTexture textureForFace(uint8_t blockId, float normalX, float normalY, float normalZ) {
    (void)normalX;
    (void)normalZ;

    if (blockId == blocks::kDirt) {
        return FaceTexture::Dirt;
    }
    if (blockId == blocks::kGrass && normalY < -0.5f) {
        return FaceTexture::Dirt;
    }
    return FaceTexture::Atlas;
}

int atlasIndexForFace(uint8_t blockId, float normalX, float normalY, float normalZ) {
    (void)normalX;

    if (blockId == blocks::kGrass) {
        if (normalY > 0.5f) {
            return 204;
        }
        if (normalY < -0.5f) {
            return 2;
        }
        return 3;
    }

    switch (blockId) {
        case blocks::kStone:
            return 1;
        case blocks::kDirt:
            return 2;
        case blocks::kCobblestone:
            return 16;
        default:
            return 1;
    }
}

void tileOrigin(int atlasIndex, float& u0, float& v0) {
    const int col = atlasIndex % 16;
    const int row = atlasIndex / 16;
    u0 = static_cast<float>(col) * kTileSize;
    v0 = 1.0f - static_cast<float>(row + 1) * kTileSize;
}

} // namespace BlockAtlas
