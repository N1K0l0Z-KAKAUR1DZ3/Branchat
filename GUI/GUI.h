#pragma once
#include "../Dependencies/imgui/imgui.h"
#include "../Dependencies/imgui/imgui_impl_glfw.h"
#include "../Dependencies/imgui/imgui_impl_opengl3.h"
#include "../Session/Session.h"
#include <GLFW/glfw3.h>

#ifndef BRANCHAT1_GUI_H
#define BRANCHAT1_GUI_H


class GUI {
public:
    static bool Initialize(int windowWidth, int windowHeight, const std::string& windowTitle);
    static void StartRenderLoop();
private:
    inline static GLFWwindow* window;
    inline static std::function<void()> deferredAction;
    enum class ModalOp { None, AddGroupRoot, RenameGroup, BranchRoot, RenameRoot, BranchChat, RenameChat };
    inline static ModalOp currentModalOp;
    inline static int modalTargetId;
    inline static char modalInputBuffer[256];
    inline static bool triggerModal = false;
    inline static int forceOpenGroupId = -1;
    inline static int forceOpenRootId = -1;
    inline static int forceOpenChatId = -1;
    static void BeginFrame();
    static void EndFrame();
    static void RenderMainApp();
    static void RenderTreeNodes(std::vector<Chat>& branches);
    static void RenderModals();
    static void ExecuteDeferredActions();
};


#endif
