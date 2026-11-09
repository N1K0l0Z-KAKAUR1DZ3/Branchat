#pragma once

#include "../Session/Session.h"
#include <GLFW/glfw3.h>
#include "../Dependencies/imgui/imgui_markdown-main/imgui_markdown.h"

#ifndef BRANCHAT1_GUI_H
#define BRANCHAT1_GUI_H


class GUI {
public:
    static bool Initialize(int windowWidth, int windowHeight, const std::string& windowTitle);
    static void StartRenderLoop();
    inline static bool scrollBottom = false;
    inline static bool scrollDown = false;
private:
    inline static float footerHeight = 150.0f;
    inline static int contexLimitBuffer;
    inline static ImFont* fancy;
    inline static ImFont* roboto;
    inline static ImFont* jbFont;
    inline static ImGui::MarkdownConfig mdConfig;
    inline static GLFWwindow* window;
    inline static std::function<void(std::string)> Action = nullptr;
    inline static bool pendingAction;
    inline static char modalInputBuffer[1024];
    inline static char chatInputBuffer[1024];
    inline static bool triggerModal = false;
    inline static int forceOpenGroupId = -1;
    inline static int forceOpenRootId = -1;
    inline static int forceOpenChatId = -1;

    // --- Images ---
    inline static std::vector<Attachment> pendingAttachments;       // attached, not sent yet
    inline static std::shared_ptr<const Attachment::Bytes> viewerImage; // image open in the viewer
    inline static bool triggerViewer = false;
    static void AddPendingAttachment(Attachment attachment);
    static void PasteImageFromClipboard();
    static void CollectPickedImages();
    static void OnFilesDropped(GLFWwindow* window, int count, const char** paths);
    static bool DrawImage(const Attachment& attachment, ImVec2 size);
    static void RenderMessageAttachments(const std::vector<Attachment>& attachments, bool alignRight);
    static float PendingAttachmentsHeight();
    static void RenderPendingAttachments();
    static void RenderImageViewer();
    static void BeginFrame();
    static void EndFrame();
    static void RenderMainApp();
    static void RenderTreeNodes(std::vector<Chat>& branches);
    static void RenderModals();
    static void ExecuteDeferredActions();
    static void InitializeMarkdown();
    static void RenderMessageContent(const std::string& content, bool isUser);
};


#endif
