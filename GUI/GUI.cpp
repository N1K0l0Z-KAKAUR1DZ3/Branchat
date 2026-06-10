//
// Created by metanika on 5/30/26.
//
#include "GUI.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "../Dependencies/imgui/IconFontCppHeaders-main/IconsFontAwesome5.h"
#include "../Dependencies/imgui/ImGuiColorTextEdit-master/TextEditor.h"

void LinkCallback(ImGui::MarkdownLinkCallbackData data_) {
    std::string url(data_.link, data_.linkLength);
    // On Windows, use ShellExecute. On Linux/macOS, use:
    std::string command = "xdg-open " + url;
    system(command.c_str());
}

void GUI::InitializeMarkdown() {
    // Configure the markdown structure
    GUI::mdConfig.linkCallback = LinkCallback;
    GUI::mdConfig.tooltipCallback = nullptr;
    GUI::mdConfig.imageCallback = nullptr;

    // Heading formats: {Font pointer, is_bulleted}
    // If you want H1 to be larger, you would pass a specific ImFont* here
    GUI::mdConfig.headingFormats[0] = { nullptr, false };
    GUI::mdConfig.headingFormats[1] = { nullptr, false };
    GUI::mdConfig.headingFormats[2] = { nullptr, false };
}

bool GUI::Initialize(int windowWidth, int windowHeight, const std::string& windowTitle) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }

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

    // IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);


    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 8.0f;
    style.FrameRounding     = 6.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 12.0f;
    style.GrabRounding      = 6.0f;

    // Add air around elements
    style.FramePadding  = ImVec2(8, 6);
    style.ItemSpacing   = ImVec2(8, 8);
    style.ItemInnerSpacing = ImVec2(6, 6);

    ImGui::StyleColorsDark(); // Start with the dark preset

    // Tweak the accents to match your branding (e.g., a nice blue)
    style.Colors[ImGuiCol_Header]       = ImVec4(0.2f, 0.4f, 0.8f, 0.6f);
    style.Colors[ImGuiCol_HeaderHovered]= ImVec4(0.2f, 0.5f, 0.9f, 0.8f);
    style.Colors[ImGuiCol_Button]       = ImVec4(0.2f, 0.2f, 0.25f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered]= ImVec4(0.3f, 0.3f, 0.35f, 1.0f);

    // InitializeMarkdown();

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
        // ==========================================
        // ADD GROUP BUTTON
        // ==========================================
        // ImVec2(-1, 30) makes the button stretch to the full width of the left panel with a height of 30
        if (ImGui::Button("+ Add Group", ImVec2(-1, 30))) {
            Action = [](const std::string& inputStr) {Base::AddGroup(inputStr);};
            triggerModal = true;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        for (auto& group : Base::groups) {
            ImGui::PushID(group.id);

            // FORCE OPEN LOGIC (GROUP)
            if (forceOpenGroupId == group.id) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Always);
                forceOpenGroupId = -1;
            }

            // ==========================================
            // DYNAMIC GROUP NODE FLAGS
            // ==========================================
            ImGuiTreeNodeFlags groupFlags = ImGuiTreeNodeFlags_OpenOnArrow;

            // If the group has no roots, make it a Leaf (removes the arrow)
            if (group.roots.empty()) {
                groupFlags |= ImGuiTreeNodeFlags_Leaf;
            }

            std::string groupLabel = group.name + "###" + std::to_string(group.id);
            bool groupOpen = ImGui::TreeNodeEx(groupLabel.c_str(), groupFlags);

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
                    Action = [&group](const std::string& inputStr) { group.AddRoot(inputStr); forceOpenGroupId = group.id;};
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
                    // SEPARATED: ACCORDION vs SELECTION LOGIC
                    // ==========================================

                    // 1. Accordion State: Which tree is currently loaded/expanded?
                    bool isExpanded = (Tree::rootId == root.id);

                    // 2. Selection State: Is this root the exact chat chatPtr is pointing to?
                    // We check activeChatId here to avoid unsafe null-pointer casting on chatPtr
                    bool isSelected = (Session::activeChatId == root.id);

                    // FORCE the arrow state to match your global tree state
                    ImGui::SetNextItemOpen(isExpanded, ImGuiCond_Always);

                    ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_OpenOnArrow;

                    // HIGHLIGHT only if it is the currently selected chat
                    if (isSelected) rootFlags |= ImGuiTreeNodeFlags_Selected;

                    if (!root.hasChildren) rootFlags |= ImGuiTreeNodeFlags_Leaf;

                    // 3. Render Node
                    std::string rootLabel = root.name + "###" + std::to_string(root.id);
                    bool rootOpen = ImGui::TreeNodeEx(rootLabel.c_str(), rootFlags);

                    // 4. Handle Interaction
                    if (ImGui::IsItemToggledOpen()) {
                        if (rootOpen) {
                            root.FocusChat();
                            if (root.hasChildren) {
                                root.LoadTree();
                            }
                        } else {
                            if (isExpanded) {
                                Tree::rootId = -1;
                            }
                        }
                    }
                    else if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
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
    // 1. HEADER (Always visible)
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Active Chat: %s", Session::chatPtr->name.c_str());
    ImGui::Separator();

    // 2. MESSAGE HISTORY (Scrollable)
    // We use a negative height to leave room for the footer (approx 140px for input + buttons)
    ImGui::BeginChild("MessageScroll", ImVec2(0, -140.0f), true);
        float maxWrapWidth = ImGui::GetWindowWidth() * 0.75f;

           for (const auto& msg : Session::chatPtr->messages) {
               // 1. Calculate how wide this specific message is
               ImVec2 textSize = ImGui::CalcTextSize(msg.Content.c_str(), NULL, false, maxWrapWidth);

               // 2. Determine alignment
               if (msg.Role == "user") {
                   // Align to right: WindowWidth - TextWidth - Padding
                   float posX = ImGui::GetWindowContentRegionMax().x - textSize.x - ImGui::GetStyle().ItemSpacing.x;
                   ImGui::SetCursorPosX(std::max(posX, 0.0f));
                   ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 1.0f, 1.0f)); // User color
               } else {
                   // Align to left (default)
                   ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f)); // AI color
               }

               // 3. Render
               ImGui::Markdown(msg.Content.c_str(), msg.Content.length(), mdConfig);

               ImGui::PopStyleColor();
               ImGui::Separator();
           }

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();

    // 3. STICKY FOOTER (Always visible at the bottom)
    ImGui::Separator();

    // Status Indicator
    if (Session::chatPtr->waitingResponse) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "AI is responding...");
    } else {
        ImGui::Spacing();
    }

    // Input Bar (Snapshot logic for safety)
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

    // Bottom Action Bar: Navigation & Options
    ImGui::Spacing();

    float dotsWidth = 40.0f;
    float navBtnWidth = 120.0f;
    float paddingRight = ImGui::GetStyle().WindowPadding.x;
    bool showNavButtons = !Session::pointingAtRoot;

    if (showNavButtons) {
        if (ImGui::Button("^ Parent", ImVec2(navBtnWidth, 0))) {
            auto tempChatPtr = static_cast<Chat*>(Session::chatPtr);
            int parentId = tempChatPtr->parentId;
            auto parentNode = Tree::FindChatById(Tree::topBranches, parentId);
            if (parentNode) parentNode->FocusChat();
            else {
                auto group = Base::FindGroup(Session::activeGroupId);
                if (group) { auto root = group->FindRootChat(parentId); if (root) root->FocusChat(); }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("^^ Root", ImVec2(navBtnWidth, 0))) {
            auto group = Base::FindGroup(Session::activeGroupId);
            if (group) { auto root = group->FindRootChat(Tree::rootId); if (root) root->FocusChat(); }
        }
    }

    // Right-aligned Options
    if (showNavButtons) ImGui::SameLine(ImGui::GetWindowWidth() - dotsWidth - paddingRight);
    else ImGui::SetCursorPosX(ImGui::GetWindowWidth() - dotsWidth - paddingRight);

    if (ImGui::Button("...##ActiveChatDots", ImVec2(dotsWidth, 0))) ImGui::OpenPopup("ActiveChatOptionsPopup");

    // Options Popup
    if (ImGui::BeginPopup("ActiveChatOptionsPopup")) {
        if (ImGui::Selectable("Create Branch")) { triggerModal = true; Action = [](const std::string& s) { Session::chatPtr->CreateBranch(s); }; }
        if (ImGui::Selectable("Rename")) { strncpy(modalInputBuffer, Session::chatPtr->name.c_str(), sizeof(modalInputBuffer)-1); triggerModal = true; Action = [](const std::string& s) { Session::chatPtr->Rename(s); }; }
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Selectable("Delete")) { pendingAction = true; Action = [](const std::string& s) { Session::chatPtr->Delete(); }; }
        ImGui::PopStyleColor();
        ImGui::EndPopup();
    }
}else {
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

        // ==========================================
        // DYNAMIC TREE NODE FLAGS
        // ==========================================
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
        if (isSelected) flags |= ImGuiTreeNodeFlags_Selected;

        // If there are no children, make it a Leaf (removes the arrow)
        if (chat.branches.empty()) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        std::string chatLabel = chat.name + "###" + std::to_string(chat.id);
        bool nodeOpen = ImGui::TreeNodeEx(chatLabel.c_str(), flags);

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
                Action = [&chat](const std::string& inputStr) {  chat.CreateBranch(inputStr); forceOpenChatId = chat.id;};
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
            // Even if it's a leaf, nodeOpen can be true when clicked,
            // but the recursive call is harmless if chat.branches is empty.
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