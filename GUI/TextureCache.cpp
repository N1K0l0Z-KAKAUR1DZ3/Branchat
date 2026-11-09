#include "TextureCache.h"
#include <GLFW/glfw3.h>
#include <map>
#include "stb_image.h"

namespace {
    struct Entry {
        ImageTexture texture;
        bool failed = false;
    };

    // Keyed by the shared bytes' control block: an entry can never be mistaken
    // for a different image, even after the original is freed.
    std::map<std::weak_ptr<const Attachment::Bytes>, Entry, std::owner_less<>> cache;

    Entry Upload(const Attachment::Bytes& bytes) {
        Entry entry;
        int w = 0, h = 0, comp = 0;
        stbi_uc* pixels = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &w, &h, &comp, 4);
        if (!pixels) {
            entry.failed = true;
            return entry;
        }

        GLint maxSize = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
        if (w > maxSize || h > maxSize) {
            stbi_image_free(pixels);
            entry.failed = true;
            return entry;
        }

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(pixels);

        entry.texture = {tex, w, h};
        return entry;
    }
}

const ImageTexture* TextureCache::Get(const std::shared_ptr<const Attachment::Bytes>& data) {
    if (!data || data->empty()) return nullptr;

    auto it = cache.find(data);
    if (it == cache.end()) {
        it = cache.emplace(data, Upload(*data)).first;
    }
    return it->second.failed ? nullptr : &it->second.texture;
}

void TextureCache::CollectGarbage() {
    for (auto it = cache.begin(); it != cache.end();) {
        if (it->first.expired()) {
            if (it->second.texture.id != 0) {
                GLuint tex = it->second.texture.id;
                glDeleteTextures(1, &tex);
            }
            it = cache.erase(it);
        } else {
            ++it;
        }
    }
}
