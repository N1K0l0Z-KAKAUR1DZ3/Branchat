#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#ifndef BRANCHAT1_ATTACHMENT_H
#define BRANCHAT1_ATTACHMENT_H

// An image attached to a message.
// The bytes are shared and never modified, so copying messages around the
// chat tree (which happens a lot) doesn't copy the image data.
struct Attachment {
    using Bytes = std::vector<std::uint8_t>;

    // Largest single image we accept. Gemini caps a whole request at 20 MB.
    static constexpr std::size_t MaxBytes = 15 * 1024 * 1024;

    int id = -1;          // DB id, -1 until saved
    std::string mimeType; // image/png, image/jpeg or image/webp
    std::string fileName;
    std::shared_ptr<const Bytes> data;

    std::size_t Size() const { return data ? data->size() : 0; }

    // Builds an attachment from raw image bytes. Formats Gemini can't read
    // (BMP, GIF, TGA, ...) are converted to PNG.
    // Throws std::runtime_error if the bytes are not a readable image.
    static Attachment FromBytes(Bytes bytes, std::string fileName);
    static Attachment FromFile(const std::string& path);
};

#endif //BRANCHAT1_ATTACHMENT_H
