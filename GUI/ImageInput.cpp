#include "ImageInput.h"
#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace {
    std::mutex resultsMutex;
    std::vector<Attachment> pickedImages;
    std::vector<std::string> pickErrors;
    std::atomic<bool> dialogOpen{false};

    bool CommandExists(const std::string& name) {
        const std::string cmd = "command -v " + name + " >/dev/null 2>&1";
        return std::system(cmd.c_str()) == 0;
    }

    // Runs a shell command and captures stdout (binary safe).
    // Stops reading once more than maxBytes arrive. Returns true on exit code 0.
    bool RunCommand(const std::string& cmd, Attachment::Bytes& out, size_t maxBytes) {
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return false;
        char buffer[64 * 1024];
        size_t n;
        while ((n = fread(buffer, 1, sizeof(buffer), pipe)) > 0) {
            out.insert(out.end(), buffer, buffer + n);
            if (out.size() > maxBytes) break;
        }
        return pclose(pipe) == 0;
    }

    std::vector<std::string> SplitLines(const Attachment::Bytes& bytes, char separator = '\n') {
        std::vector<std::string> lines;
        std::string current;
        for (auto c : bytes) {
            if (c == static_cast<std::uint8_t>(separator) || c == '\n') {
                if (!current.empty()) lines.push_back(current);
                current.clear();
            } else if (c != '\r') {
                current.push_back(static_cast<char>(c));
            }
        }
        if (!current.empty()) lines.push_back(current);
        return lines;
    }

    // MIME types come from whatever app owns the clipboard and go into a shell
    // command, so only allow plain "image/<subtype>" characters.
    bool IsSafeImageMime(const std::string& mime) {
        if (mime.rfind("image/", 0) != 0 || mime.size() > 64) return false;
        for (char c : mime) {
            if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '/' || c == '.' || c == '+' || c == '-'))
                return false;
        }
        return true;
    }

    std::string ExtensionFor(const std::string& mime) {
        if (mime == "image/png") return ".png";
        if (mime == "image/jpeg") return ".jpg";
        if (mime == "image/webp") return ".webp";
        if (mime == "image/bmp") return ".bmp";
        if (mime == "image/gif") return ".gif";
        return ".img";
    }

    std::vector<std::string> PickFiles() {
        const std::string exts = "*.png *.jpg *.jpeg *.webp *.bmp *.gif";
        std::string cmd;
        char separator = '\n';
        if (CommandExists("zenity")) {
            cmd = "zenity --file-selection --multiple --separator='|' --title='Attach images' "
                  "--file-filter='Images | " + exts + "' --file-filter='All files | *' 2>/dev/null";
            separator = '|';
        } else if (CommandExists("kdialog")) {
            cmd = "kdialog --getopenfilename --multiple --separate-output . 'Images (" + exts + ")' 2>/dev/null";
        } else {
            throw std::runtime_error("No file picker found. Install zenity, or drag image files onto the window.");
        }

        Attachment::Bytes output;
        // A non-zero exit code means the user cancelled
        if (!RunCommand(cmd, output, 1024 * 1024)) return {};
        return SplitLines(output, separator);
    }
}

std::optional<Attachment> ImageInput::ReadClipboardImage() {
    const bool wayland = std::getenv("WAYLAND_DISPLAY") != nullptr;
    const bool x11 = std::getenv("DISPLAY") != nullptr;

    std::string listCmd, readCmd;
    if (wayland && CommandExists("wl-paste")) {
        listCmd = "wl-paste --list-types 2>/dev/null";
        readCmd = "wl-paste --no-newline --type ";
    } else if (x11 && CommandExists("xclip")) {
        listCmd = "xclip -selection clipboard -t TARGETS -o 2>/dev/null";
        readCmd = "xclip -selection clipboard -o -t ";
    } else if (wayland) {
        throw std::runtime_error("To paste screenshots, install wl-clipboard (sudo apt install wl-clipboard).");
    } else {
        throw std::runtime_error("To paste screenshots, install xclip (sudo apt install xclip).");
    }

    Attachment::Bytes typeList;
    if (!RunCommand(listCmd, typeList, 64 * 1024)) return std::nullopt; // empty clipboard

    std::string chosen;
    for (const auto& type : SplitLines(typeList)) {
        if (!IsSafeImageMime(type)) continue;
        if (type == "image/png") { chosen = type; break; } // lossless, always preferred
        if (chosen.empty()) chosen = type;
    }
    if (chosen.empty()) return std::nullopt;

    Attachment::Bytes data;
    if (!RunCommand(readCmd + chosen + " 2>/dev/null", data, Attachment::MaxBytes) || data.empty()) {
        if (data.size() > Attachment::MaxBytes) {
            throw std::runtime_error("The image in the clipboard is too large.");
        }
        throw std::runtime_error("Could not read the image from the clipboard.");
    }
    return Attachment::FromBytes(std::move(data), "pasted-image" + ExtensionFor(chosen));
}

void ImageInput::OpenFileDialogAsync() {
    if (dialogOpen.exchange(true)) return; // already open

    std::thread([] {
        std::vector<Attachment> images;
        std::vector<std::string> errors;
        try {
            for (const auto& path : PickFiles()) {
                try {
                    images.push_back(Attachment::FromFile(path));
                } catch (const std::exception& e) {
                    errors.emplace_back(e.what());
                }
            }
        } catch (const std::exception& e) {
            errors.emplace_back(e.what());
        }
        {
            std::lock_guard<std::mutex> lock(resultsMutex);
            for (auto& img : images) pickedImages.push_back(std::move(img));
            for (auto& err : errors) pickErrors.push_back(std::move(err));
        }
        dialogOpen = false;
    }).detach();
}

bool ImageInput::IsFileDialogOpen() {
    return dialogOpen;
}

void ImageInput::TakeFileDialogResults(std::vector<Attachment>& images, std::vector<std::string>& errors) {
    std::lock_guard<std::mutex> lock(resultsMutex);
    for (auto& img : pickedImages) images.push_back(std::move(img));
    for (auto& err : pickErrors) errors.push_back(std::move(err));
    pickedImages.clear();
    pickErrors.clear();
}
