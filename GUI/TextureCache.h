#pragma once
#include <memory>
#include "../Models/Headers/Attachment.h"

#ifndef BRANCHAT1_TEXTURECACHE_H
#define BRANCHAT1_TEXTURECACHE_H

struct ImageTexture {
    unsigned int id = 0; // OpenGL texture name
    int width = 0;
    int height = 0;
};

// Turns attachment bytes into OpenGL textures for display, once per image.
// UI thread only (needs the GL context).
namespace TextureCache {
    // Returns the texture for these bytes, decoding and uploading on first use.
    // Returns nullptr if the image can't be shown (e.g. WEBP, which stb can't decode).
    const ImageTexture* Get(const std::shared_ptr<const Attachment::Bytes>& data);

    // Frees textures whose images are gone (chat deleted, tree reloaded, ...).
    // Call once per frame.
    void CollectGarbage();
}

#endif //BRANCHAT1_TEXTURECACHE_H
