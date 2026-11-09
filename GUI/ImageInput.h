#pragma once
#include <optional>
#include <string>
#include <vector>
#include "../Models/Headers/Attachment.h"

#ifndef BRANCHAT1_IMAGEINPUT_H
#define BRANCHAT1_IMAGEINPUT_H

// Getting images into the app from outside: clipboard and file picker.
// Linux only for now (uses wl-clipboard / xclip and zenity / kdialog).
namespace ImageInput {
    // Reads an image from the system clipboard.
    // Returns nullopt when the clipboard holds no image.
    // Throws std::runtime_error when no clipboard tool is installed or the image is unusable.
    std::optional<Attachment> ReadClipboardImage();

    // Opens the system file picker on a background thread so the UI keeps drawing.
    void OpenFileDialogAsync();
    bool IsFileDialogOpen();

    // Moves images picked (and errors raised) since the last call into the given lists.
    // Call once per frame from the UI thread.
    void TakeFileDialogResults(std::vector<Attachment>& images, std::vector<std::string>& errors);
}

#endif //BRANCHAT1_IMAGEINPUT_H
