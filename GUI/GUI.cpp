//
// Created by metanika on 5/30/26.
//

#include "GUI.h"

bool GUI::Initialize(int windowWidth, int windowHeight, const std::string& windowTitle) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window = glfwCreateWindow(windowWidth, windowHeight, windowTitle.c_str(), nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    return true;
}

void GUI::StartRenderLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        BeginFrame();

        RenderMainApp();
        RenderModals(); // Draw active popups on top

        EndFrame();

        // Safety Net: Execute structural changes outside the render loop!
        ExecuteDeferredActions();
    }
}

void GUI::BeginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GUI::EndFrame() {
    ImGui::Render();

    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}

void GUI::RenderMainApp() {
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
    ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
    ImGui::Begin("Branchat", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

    if (ImGui::BeginTable("Splitter", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableNextRow();

        // ==========================================
        // LEFT PANEL: NAVIGATION
        // ==========================================
        ImGui::TableSetColumnIndex(0);
        ImGui::BeginChild("LeftPanel", ImVec2(0, 0), false);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 10));

        for (auto& group : Base::groups) {
            ImGui::PushID(group.id);

            // FORCE OPEN LOGIC (GROUP)
            if (forceOpenGroupId == group.id) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Always);
                forceOpenGroupId = -1;
            }

            std::string groupLabel = group.name + "###" + std::to_string(group.id);
            bool groupOpen = ImGui::TreeNodeEx(groupLabel.c_str(), ImGuiTreeNodeFlags_OpenOnArrow);

            // HOVER LOGIC: GROUP
            ImVec2 groupMin = ImGui::GetItemRectMin();
            ImVec2 groupMax = ImGui::GetItemRectMax();
            groupMax.x += 50.0f;
            if (ImGui::IsMouseHoveringRect(groupMin, groupMax) || ImGui::IsPopupOpen("GroupOptions")) {
                ImGui::SameLine();
                if (ImGui::Button("...")) ImGui::OpenPopup("GroupOptions");
            }

            if (ImGui::BeginPopup("GroupOptions")) {
                if (ImGui::Selectable("Add Root")) {
                    currentModalOp = ModalOp::AddGroupRoot; modalTargetId = group.id;
                    modalInputBuffer[0] = '\0'; triggerModal = true;
                    forceOpenGroupId = group.id;
                }
                if (ImGui::Selectable("Rename")) {
                    currentModalOp = ModalOp::RenameGroup; modalTargetId = group.id;
                    strncpy(modalInputBuffer, group.name.c_str(), sizeof(modalInputBuffer) - 1);
                    modalInputBuffer[sizeof(modalInputBuffer) - 1] = '\0';
                    triggerModal = true;
                }
                if (ImGui::Selectable("Delete")) { deferredAction = [&group]() { group.Delete(); }; }
                ImGui::EndPopup();
            }

            if (groupOpen) {
                for (auto& root : group.roots) {
                    ImGui::PushID(root.id);

                    // ACCORDION & SELECTION LOGIC
                    // We force the node open if Tree::rootId matches this root
                    ImGui::SetNextItemOpen(Tree::rootId == root.id, ImGuiCond_Always);

                    bool isSelected = (Tree::rootId == root.id);

                    ImGuiTreeNodeFlags rootFlags = (isSelected ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
                    if (!root.hasChildren) rootFlags |= ImGuiTreeNodeFlags_Leaf;

                    std::string rootLabel = root.name + "###" + std::to_string(root.id);
                    bool rootOpen = ImGui::TreeNodeEx(rootLabel.c_str(), rootFlags);

                    // Handle Focus
                    if (ImGui::IsItemClicked()) {
                        deferredAction = [&root]() { root.FocusChat(); };
                    }
                    // Handle Load
                    else if (rootOpen && root.hasChildren &&  Tree::rootId != root.id) {
                        deferredAction = [&root]() { root.LoadTree(); };
                    }

                    // HOVER LOGIC: ROOT CHAT
                    ImVec2 rootMin = ImGui::GetItemRectMin();
                    ImVec2 rootMax = ImGui::GetItemRectMax();
                    rootMax.x += 50.0f;
                    if (ImGui::IsMouseHoveringRect(rootMin, rootMax) || ImGui::IsPopupOpen("RootOptions")) {
                        ImGui::SameLine();
                        if (ImGui::Button("...")) ImGui::OpenPopup("RootOptions");
                    }

                    if (ImGui::BeginPopup("RootOptions")) {
                        if (ImGui::Selectable("Create Branch")) {
                            currentModalOp = ModalOp::BranchRoot; modalTargetId = root.id;
                            modalInputBuffer[0] = '\0'; triggerModal = true;
                        }
                        if (ImGui::Selectable("Rename")) {
                            currentModalOp = ModalOp::RenameRoot; modalTargetId = root.id;
                            strncpy(modalInputBuffer, root.name.c_str(), sizeof(modalInputBuffer) - 1);
                            modalInputBuffer[sizeof(modalInputBuffer) - 1] = '\0';
                            triggerModal = true;
                        }
                        if (ImGui::Selectable("Delete")) { deferredAction = [&root]() { root.Delete(); }; }
                        ImGui::EndPopup();
                    }

                    if (rootOpen) {
                        if (Tree::rootId == root.id) {
                            RenderTreeNodes(Tree::topBranches);
                        }
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
        ImGui::PopStyleVar();
        ImGui::EndChild();

        // ==========================================
        // RIGHT PANEL: CHAT INTERFACE
        // ==========================================
        ImGui::TableSetColumnIndex(1);
        ImGui::BeginChild("RightPanel", ImVec2(0, 0), false);

        if (Session::chatPtr != nullptr) {
            // [Your existing rendering code for Session::chatPtr...]
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Active Chat: %s", Session::chatPtr->name.c_str());
            ImGui::Separator();
            // ... (rest of messages loop)
        } else {
            ImGui::TextDisabled("Select a chat from the left panel to begin.");
        }
        ImGui::EndChild();
        ImGui::EndTable();
    }
    RenderModals();
    ImGui::End();
}

void GUI::RenderTreeNodes(std::vector<Chat>& branches) {
    for (auto& chat : branches) {
        ImGui::PushID(chat.id);
        bool isSelected = (Session::activeChatId == chat.id && !Session::pointingAtRoot);

        // FORCE OPEN LOGIC (CHAT)
        if (forceOpenChatId == chat.id) {
            ImGui::SetNextItemOpen(true, ImGuiCond_Always);
            forceOpenChatId = -1;
        }

        std::string chatLabel = chat.name + "###" + std::to_string(chat.id);
        bool nodeOpen = ImGui::TreeNodeEx(chatLabel.c_str(),
            (isSelected ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow);

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            deferredAction = [&chat]() { chat.FocusChat(); };
        }

        // ==========================================
        // HOVER LOGIC: NESTED CHAT
        // ==========================================
        ImVec2 chatMin = ImGui::GetItemRectMin();
        ImVec2 chatMax = ImGui::GetItemRectMax();
        chatMax.x += 50.0f; // Stretch hover zone

        if (ImGui::IsMouseHoveringRect(chatMin, chatMax) || ImGui::IsPopupOpen("ChatOptions")) {
            ImGui::SameLine();
            if (ImGui::Button("...")) ImGui::OpenPopup("ChatOptions");
        }
        // ==========================================

        if (ImGui::BeginPopup("ChatOptions")) {
            if (ImGui::Selectable("Create Branch")) {
                currentModalOp = ModalOp::BranchChat; modalTargetId = chat.id;
                modalInputBuffer[0] = '\0'; triggerModal = true;
                forceOpenChatId = chat.id;
            }
            if (ImGui::Selectable("Rename")) {
                currentModalOp = ModalOp::RenameChat; modalTargetId = chat.id;
                strncpy(modalInputBuffer, chat.name.c_str(), sizeof(modalInputBuffer) - 1);
                modalInputBuffer[sizeof(modalInputBuffer) - 1] = '\0';
                triggerModal = true;
            }
            if (ImGui::Selectable("Delete")) { deferredAction = [&chat]() { chat.Delete(); }; }
            ImGui::EndPopup();
        }

        if (nodeOpen) {
            RenderTreeNodes(chat.branches);
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}

void GUI::RenderModals() {

    if (triggerModal) {
        std::cout << "DEBUG: Trigger detected! Attempting to open popup." << std::endl;
        ImGui::OpenPopup("##UniqueInputModal");
        triggerModal = false; // Reset the flag immediately
    }

    if (ImGui::BeginPopupModal("##UniqueInputModal", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter name:");
        if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();

        bool enterPressed = ImGui::InputText("##NameInput", modalInputBuffer, IM_ARRAYSIZE(modalInputBuffer), ImGuiInputTextFlags_EnterReturnsTrue);

        if (ImGui::Button("OK", ImVec2(120, 0)) || enterPressed) {
            std::string inputStr(modalInputBuffer);
            if (!inputStr.empty()) {
                // Determine what to do based on the active op
                ModalOp op = currentModalOp; // Capture locally
                int id = modalTargetId;      // Capture locally

                deferredAction = [op, id, inputStr]() {
                    if (op == ModalOp::AddGroupRoot) {
                        Group* g = Base::FindGroup(id);
                        if (g) g->AddRoot(inputStr);
                    } else if (op == ModalOp::RenameGroup) {
                        Group* g = Base::FindGroup(id);
                        if (g) g->Rename(inputStr);
                    } else if (op == ModalOp::BranchRoot) {
                        std::cout << "triggered branchRoot\n";
                        RootChat* r = nullptr;
                        for (auto& g : Base::groups) {
                            r = g.FindRootChat(id);
                            if (r) break;
                        }
                        if (r) r->CreateBranch(inputStr);
                    }else if (op == ModalOp::RenameRoot) {
                        Group* g = Base::FindGroup(Session::activeGroupId);
                        if (g) { RootChat* r = g->FindRootChat(id); if (r) r->Rename(inputStr); }
                    } else if (op == ModalOp::BranchChat) {
                        std::cout << "GUI/render modals using FindChatById \n";
                        Chat* c = Tree::FindChatById(Tree::topBranches, id);
                        if (c) c->CreateBranch(inputStr);
                    } else if (op == ModalOp::RenameChat) {
                        std::cout << "GUI/render modals using FindChatById \n";
                        Chat* c = Tree::FindChatById(Tree::topBranches, id);
                        if (c) c->Rename(inputStr);
                    }
                };
            }
            ImGui::CloseCurrentPopup();
            currentModalOp = ModalOp::None;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            currentModalOp = ModalOp::None;
        }
        ImGui::EndPopup();
    }
}

void GUI::ExecuteDeferredActions() {
    if (deferredAction != nullptr) {
        deferredAction();
        deferredAction = nullptr;
    }
}