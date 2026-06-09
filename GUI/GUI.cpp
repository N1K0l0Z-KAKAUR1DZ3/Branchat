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
    // IMGUI_CHECKVERSION();
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
        RenderModals();
        EndFrame();
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
    // ========================================================
    // GLOBAL ERROR OVERLAY (Pin this to the very top)
    // ========================================================
    {
        // Use a lock to safely check the static global string from the UI thread
        std::lock_guard<std::mutex> lock(Session::globalErrorMutex);
        if (!Session::globalErrorMessage.empty()) {

            // 1. Styling
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.6f, 0.1f, 0.1f, 0.9f));

            // 2. Create the child window
            ImGui::BeginChild("ErrorOverlay", ImVec2(0, 50), true);

            // 3. Define layout constants
            const float buttonWidth = 80.0f;
            const float padding = 10.0f;
            const float availableWidth = ImGui::GetWindowWidth();

            // 4. Render the Button first (Right-aligned)
            ImGui::SameLine(availableWidth - buttonWidth - padding);
            if (ImGui::Button("Dismiss", ImVec2(buttonWidth, 30))) {
                Session::globalErrorMessage = "";
            }

            // 5. Render Text (constrained to the left of the button)
            ImGui::SetCursorPos(ImVec2(padding, padding));

            // Set the wrap limit so text doesn't flow behind the button
            ImGui::PushTextWrapPos(availableWidth - buttonWidth - (padding * 2));
            ImGui::TextUnformatted(("!!! ERROR: " + Session::globalErrorMessage).c_str());
            ImGui::PopTextWrapPos();

            ImGui::EndChild();
            ImGui::PopStyleColor();

            // 6. Add space after banner
            ImGui::Spacing();
        }
    }
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
                    forceOpenGroupId = group.id;
                    triggerModal = true;
                }
                if (ImGui::Selectable("Rename")) {
                    Action = [&group](const std::string& inputStr) { group.Rename(inputStr); };
                    triggerModal = true;
                }
                if (ImGui::Selectable("Delete")) {
                    Action = [&group](std::string ff) { group.Delete(); };
                    pendingAction = true;
                }
                ImGui::EndPopup();
            }

            if (groupOpen) {
                for (auto& root : group.roots) {
                    ImGui::PushID(root.id);

                    // ==========================================
                    // FIXED: ACCORDION & SELECTION LOGIC
                    // ==========================================
                    bool isSelected = (Tree::rootId == root.id);

                    ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_OpenOnArrow;
                    if (isSelected) rootFlags |= ImGuiTreeNodeFlags_Selected;
                    if (!root.hasChildren) rootFlags |= ImGuiTreeNodeFlags_Leaf;

                    // 3. Render Node
                    std::string rootLabel = root.name + "###" + std::to_string(root.id);
                    bool rootOpen = ImGui::TreeNodeEx(rootLabel.c_str(), rootFlags);

                    // 4. Handle Interaction
                    if (ImGui::IsItemToggledOpen()) {
                        if (rootOpen && root.hasChildren && !isSelected) {
                            root.LoadTree();
                        }
                    }
                    else if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                        // Only here do we set the selection
                        root.FocusChat();
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
                            Action = [&root](const std::string& inputStr) { root.CreateBranch(inputStr); };
                            triggerModal = true;
                        }
                        if (ImGui::Selectable("Rename")) {
                            Action = [&root](const std::string& inputStr) { root.Rename(inputStr); };
                            triggerModal = true;
                        }
                        if (ImGui::Selectable("Delete")) {
                            Action = [&root](const std::string& inputStr) { root.Delete(); };
                            pendingAction = true;
                        }
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
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Active Chat: %s", Session::chatPtr->name.c_str());
    ImGui::Separator();

    // 1. Message History
    ImGui::BeginChild("MessageScroll", ImVec2(0, -60), true);
        float maxWrapWidth = ImGui::GetWindowWidth() * 0.75f;

        for (const auto& msg : Session::chatPtr->messages) {
            ImVec2 textSize = ImGui::CalcTextSize(msg.Content.c_str(), NULL, false, maxWrapWidth);

            if (msg.Role == "user") {
                float startPosX = ImGui::GetWindowWidth() - textSize.x - 20;
                ImGui::SetCursorPosX(std::max(startPosX, 0.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 1.0f, 1.0f));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
            }

            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + maxWrapWidth);
            ImGui::Text("%s", msg.Content.c_str());
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();
            ImGui::Separator();
        }

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();

    // 2. Status Indicator & Input Bar
    ImGui::Separator();

    // Show indicator if waiting
    if (Session::chatPtr->waitingResponse) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "AI is responding...");
    } else {
        ImGui::Spacing(); // Maintain spacing
    }

    // Disable input while waiting for response
    bool isWaiting = Session::chatPtr->waitingResponse;
    if (isWaiting) ImGui::BeginDisabled();

    ImGui::SetNextItemWidth(-ImGui::GetFrameHeightWithSpacing() * 4.0f);
    bool enterPressed = ImGui::InputText("##ChatInput", chatInputBuffer, IM_ARRAYSIZE(chatInputBuffer), ImGuiInputTextFlags_EnterReturnsTrue);

    ImGui::SameLine();
    if (ImGui::Button("Send", ImVec2(-1, 0)) || enterPressed) {
        if (chatInputBuffer[0] != '\0') {
            Session::chatPtr->SendPrompt(std::string(chatInputBuffer));
            chatInputBuffer[0] = '\0';
        }
    }

    if (isWaiting) ImGui::EndDisabled();

} else {
    ImGui::TextDisabled("Select a chat from the left panel to begin.");
}
        ImGui::EndChild();
        ImGui::EndTable();
    }
    ImGui::End();
}

void GUI::RenderTreeNodes(std::vector<Chat>& branches) {
    for (auto& chat : branches) {
        ImGui::PushID(chat.id);
        bool isSelected = (Session::activeChatId == chat.id);

        // FORCE OPEN LOGIC (CHAT)
        if (forceOpenChatId == chat.id) {
            ImGui::SetNextItemOpen(true, ImGuiCond_Always);
            forceOpenChatId = -1;
        }

        std::string chatLabel = chat.name + "###" + std::to_string(chat.id);
        bool nodeOpen = ImGui::TreeNodeEx(chatLabel.c_str(),  (isSelected ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow);

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            chat.FocusChat();
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
                triggerModal = true;
                forceOpenChatId = chat.id;
                Action = [&chat](const std::string& inputStr) {  chat.CreateBranch(inputStr); };
            }
            if (ImGui::Selectable("Rename")) {
                strncpy(modalInputBuffer, chat.name.c_str(), sizeof(modalInputBuffer) - 1);
                triggerModal = true;
                Action = [&chat](const std::string& inputStr) {  chat.Rename(inputStr); };
            }
            if (ImGui::Selectable("Delete")) {
                Action = [&chat](const std::string& inputStr) {  chat.Delete(); };
                pendingAction = true;
            }
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
        // std::cout << "DEBUG: Trigger detected! Attempting to open popup." << std::endl;
        ImGui::OpenPopup("##UniqueInputModal");
        triggerModal = false; // Reset the flag immediately
    }

    if (ImGui::BeginPopupModal("##UniqueInputModal", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter name:");
        if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();

        bool enterPressed = ImGui::InputText("##NameInput", modalInputBuffer, IM_ARRAYSIZE(modalInputBuffer), ImGuiInputTextFlags_EnterReturnsTrue);

        if (ImGui::Button("OK", ImVec2(120, 0)) || enterPressed) {
            if (modalInputBuffer[0] != '\0') {
                pendingAction = true;
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void GUI::ExecuteDeferredActions() {
    if (pendingAction) {
        Action(std::string(modalInputBuffer));
        pendingAction = false;
        Action = nullptr;
        modalInputBuffer[0] = '\0';
    }
}