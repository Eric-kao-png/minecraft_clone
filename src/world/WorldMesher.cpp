#include "world/WorldMesher.h"

#include "world/BlockAtlas.h"
#include "world/BlockId.h"
#include "world/Chunk.h"
#include "world/ChunkManager.h"
#include "world/TerrainGenerator.h"

#include <vector>

namespace {

struct FaceDef {
    int dx, dy, dz;
    float nx, ny, nz;
    float v[6][5];
};

constexpr FaceDef kFaces[6] = {
    {-0, 0, -1, 0, 0, -1,
     {{-0.5f, -0.5f, -0.5f, 0, 0},
      {0.5f, -0.5f, -0.5f, 1, 0},
      {0.5f, 0.5f, -0.5f, 1, 1},
      {0.5f, 0.5f, -0.5f, 1, 1},
      {-0.5f, 0.5f, -0.5f, 0, 1},
      {-0.5f, -0.5f, -0.5f, 0, 0}}},
    {0, 0, 1, 0, 0, 1,
     {{0.5f, -0.5f, 0.5f, 0, 0},
      {-0.5f, -0.5f, 0.5f, 1, 0},
      {-0.5f, 0.5f, 0.5f, 1, 1},
      {-0.5f, 0.5f, 0.5f, 1, 1},
      {0.5f, 0.5f, 0.5f, 0, 1},
      {0.5f, -0.5f, 0.5f, 0, 0}}},
    {-1, 0, 0, -1, 0, 0,
     {{-0.5f, -0.5f, 0.5f, 0, 0},
      {-0.5f, -0.5f, -0.5f, 1, 0},
      {-0.5f, 0.5f, -0.5f, 1, 1},
      {-0.5f, 0.5f, -0.5f, 1, 1},
      {-0.5f, 0.5f, 0.5f, 0, 1},
      {-0.5f, -0.5f, 0.5f, 0, 0}}},
    {1, 0, 0, 1, 0, 0,
     {{0.5f, -0.5f, -0.5f, 0, 0},
      {0.5f, -0.5f, 0.5f, 1, 0},
      {0.5f, 0.5f, 0.5f, 1, 1},
      {0.5f, 0.5f, 0.5f, 1, 1},
      {0.5f, 0.5f, -0.5f, 0, 1},
      {0.5f, -0.5f, -0.5f, 0, 0}}},
    {0, -1, 0, 0, -1, 0,
     {{-0.5f, -0.5f, -0.5f, 0, 1},
      {0.5f, -0.5f, -0.5f, 1, 1},
      {0.5f, -0.5f, 0.5f, 1, 0},
      {0.5f, -0.5f, 0.5f, 1, 0},
      {-0.5f, -0.5f, 0.5f, 0, 0},
      {-0.5f, -0.5f, -0.5f, 0, 1}}},
    {0, 1, 0, 0, 1, 0,
     {{-0.5f, 0.5f, -0.5f, 0, 1},
      {0.5f, 0.5f, -0.5f, 1, 1},
      {0.5f, 0.5f, 0.5f, 1, 0},
      {0.5f, 0.5f, 0.5f, 1, 0},
      {-0.5f, 0.5f, 0.5f, 0, 0},
      {-0.5f, 0.5f, -0.5f, 0, 1}}},
};

void pushVertex(std::vector<float>& out, float px, float py, float pz, float nx, float ny, float nz, float u,
                float v, float textureLayer) {
    out.push_back(px);
    out.push_back(py);
    out.push_back(pz);
    out.push_back(nx);
    out.push_back(ny);
    out.push_back(nz);
    out.push_back(u);
    out.push_back(v);
    out.push_back(textureLayer);
}

} // namespace

void WorldMesher::buildChunkMesh(Chunk& chunk, const ChunkManager& chunkManager,
                                 const TerrainGenerator& generator) const {
    chunk.mesh.clear();

    for (int y = 0; y < Chunk::SIZE_Y; ++y) {
        for (int lz = 0; lz < Chunk::SIZE_Z; ++lz) {
            for (int lx = 0; lx < Chunk::SIZE_X; ++lx) {
                const uint8_t blockID = chunk.at(lx, y, lz);
                if (blockID == blocks::kAir) {
                    continue;
                }

                const int wx = chunk.coord.x * Chunk::SIZE_X + lx;
                const int wz = chunk.coord.z * Chunk::SIZE_Z + lz;

                for (const auto& face : kFaces) {
                    if (chunkManager.hasBlockGlobal(wx + face.dx, y + face.dy, wz + face.dz, generator)) {
                        continue;
                    }

                    const BlockAtlas::FaceTexture faceTexture =
                        BlockAtlas::textureForFace(blockID, face.nx, face.ny, face.nz);
                    const float textureLayer =
                        (faceTexture == BlockAtlas::FaceTexture::Dirt) ? 1.0f : 0.0f;

                    float u0 = 0.0f;
                    float v0 = 0.0f;
                    if (faceTexture == BlockAtlas::FaceTexture::Atlas) {
                        const int atlasIndex =
                            BlockAtlas::atlasIndexForFace(blockID, face.nx, face.ny, face.nz);
                        BlockAtlas::tileOrigin(atlasIndex, u0, v0);
                    }

                    for (int i = 0; i < 6; ++i) {
                        float u = 0.0f;
                        float v = 0.0f;
                        if (faceTexture == BlockAtlas::FaceTexture::Dirt) {
                            u = face.v[i][3];
                            v = face.v[i][4];
                        } else {
                            u = u0 + face.v[i][3] * BlockAtlas::kTileSize;
                            v = v0 + face.v[i][4] * BlockAtlas::kTileSize;
                        }
                        pushVertex(chunk.mesh, wx + face.v[i][0], y + face.v[i][1], wz + face.v[i][2], face.nx,
                                   face.ny, face.nz, u, v, textureLayer);
                    }
                }
            }
        }
    }
    chunk.dirty = false;
}
