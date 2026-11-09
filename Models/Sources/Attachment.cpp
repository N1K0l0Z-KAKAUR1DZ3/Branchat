#include "../Headers/Attachment.h"
#include <filesystem>
#include <fstream>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace {
    // Returns the MIME type for formats Gemini accepts directly, or "" otherwise.
    std::string DetectNativeMime(const Attachment::Bytes& b) {
        if (b.size() >= 8 && b[0] == 0x89 && b[1] == 'P' && b[2] == 'N' && b[3] == 'G')
            return "image/png";
        if (b.size() >= 3 && b[0] == 0xFF && b[1] == 0xD8 && b[2] == 0xFF)
            return "image/jpeg";
        if (b.size() >= 12 && std::equal(b.begin(), b.begin() + 4, "RIFF") &&
            std::equal(b.begin() + 8, b.begin() + 12, "WEBP"))
            return "image/webp";
        return "";
    }

    void AppendToBytes(void* context, void* data, int size) {
        auto* out = static_cast<Attachment::Bytes*>(context);
        auto* p = static_cast<std::uint8_t*>(data);
        out->insert(out->end(), p, p + size);
    }

    std::string SizeLimitText() {
        return std::to_string(Attachment::MaxBytes / (1024 * 1024)) + " MB";
    }
}

Attachment Attachment::FromBytes(Bytes bytes, std::string fileName) {
    if (bytes.empty()) {
        throw std::runtime_error("Image '" + fileName + "' is empty.");
    }
    if (bytes.size() > MaxBytes) {
        throw std::runtime_error("Image '" + fileName + "' is larger than " + SizeLimitText() + ".");
    }

    std::string mime = DetectNativeMime(bytes);
    if (mime.empty()) {
        // Not PNG/JPEG/WEBP: decode with stb and re-encode as PNG.
        int w = 0, h = 0, comp = 0;
        stbi_uc* pixels = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &w, &h, &comp, 4);
        if (!pixels) {
            throw std::runtime_error("'" + fileName + "' is not a supported image (use PNG, JPEG, WEBP, BMP or GIF).");
        }
        Bytes png;
        const int ok = stbi_write_png_to_func(AppendToBytes, &png, w, h, 4, pixels, w * 4);
        stbi_image_free(pixels);
        if (!ok || png.empty()) {
            throw std::runtime_error("Failed to convert '" + fileName + "' to PNG.");
        }
        if (png.size() > MaxBytes) {
            throw std::runtime_error("Image '" + fileName + "' is larger than " + SizeLimitText() + " after conversion.");
        }
        bytes = std::move(png);
        mime = "image/png";
        fileName = std::filesystem::path(fileName).replace_extension(".png").string();
    }

    Attachment attachment;
    attachment.mimeType = std::move(mime);
    attachment.fileName = std::move(fileName);
    attachment.data = std::make_shared<const Bytes>(std::move(bytes));
    return attachment;
}

Attachment Attachment::FromFile(const std::string& path) {
    const std::string name = std::filesystem::path(path).filename().string();

    std::error_code ec;
    const auto fileSize = std::filesystem::file_size(path, ec);
    if (ec) {
        throw std::runtime_error("Cannot read '" + path + "': " + ec.message());
    }
    if (fileSize > MaxBytes) {
        throw std::runtime_error("Image '" + name + "' is larger than " + SizeLimitText() + ".");
    }

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open '" + path + "'.");
    }
    Bytes bytes(fileSize);
    file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(fileSize));
    if (!file) {
        throw std::runtime_error("Failed to read '" + path + "'.");
    }
    return FromBytes(std::move(bytes), name);
}
