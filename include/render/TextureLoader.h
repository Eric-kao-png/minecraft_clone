#pragma once

namespace render {

enum class TextureFilter {
    Smooth,   // Linear filtering + mipmaps
    PixelArt, // Nearest-neighbor, sharp block textures
};

unsigned int loadTexture2D(const char* path, TextureFilter filter = TextureFilter::Smooth);

} // namespace render
