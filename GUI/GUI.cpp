//
// Created by metanika on 5/30/26.
//
#include "GUI.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "../Dependencies/imgui/IconFontCppHeaders-main/IconsFontAwesome5.h"
#include "../Dependencies/imgui/ImGuiColorTextEdit-master/TextEditor.h"
#include "ImageInput.h"
#include "TextureCache.h"

void LinkCallback(ImGui::MarkdownLinkCallbackData data_) {
    std::string url(data_.link, data_.linkLength);
    // On Windows, use ShellExecute. On Linux/macOS, use:
    std::string command = "xdg-open " + url;
    system(command.c_str());
}

void GUI::InitializeMarkdown() {
    mdConfig.linkCallback = LinkCallback;
    mdConfig.tooltipCallback = nullptr;
    mdConfig.imageCallback = nullptr;

    // --- CORRECTED: Use EMPHASIS and check the level ---
    mdConfig.formatCallback = [](const ImGui::MarkdownFormatInfo &markdownFormatInfo_, bool start_) {
        // 1. Is the parser looking at emphasis text (* or **)?
        if (markdownFormatInfo_.type == ImGui::MarkdownFormatType::EMPHASIS) {
            // 2. Is it level 2? (Double asterisks = Bold)
            if (markdownFormatInfo_.level == 2) {
                if (start_) {
                    ImGui::PushFont(GUI::roboto);
                } else {
                    ImGui::PopFont();
                }
            }
        }

        // 3. ALWAYS call the default callback so colors and other formats still work
        ImGui::defaultMarkdownFormatCallback(markdownFormatInfo_, start_);
    };
}

void GUI::RenderMessageContent(const std::string &content, bool isUser) {
    size_t pos = 0;
    while (pos < content.length()) {
        // 1. Look for the start of a code block
        size_t codeStart = content.find("```", pos);

        // 2. Render any standard markdown that comes BEFORE the code block
        if (codeStart != std::string::npos) {
            if (codeStart > pos) {
                std::string text = content.substr(pos, codeStart - pos);
                ImGui::Markdown(text.c_str(), text.length(), GUI::mdConfig);
            }
        } else {
            // No more code blocks found. Render the remaining text and exit.
            std::string text = content.substr(pos);
            ImGui::Markdown(text.c_str(), text.length(), GUI::mdConfig);
            break;
        }

        // 3. Find where this code block ends
        size_t codeEnd = content.find("```", codeStart + 3);
        if (codeEnd == std::string::npos) codeEnd = content.length(); // Failsafe for unclosed blocks

        // 4. Extract the code and the language identifier (e.g., "cpp")
        std::string codeSection = content.substr(codeStart + 3, codeEnd - (codeStart + 3));
        size_t firstNewline = codeSection.find('\n');
        std::string language = "code";
        std::string actualCode = codeSection;

        if (firstNewline != std::string::npos && firstNewline < 20) {
            language = codeSection.substr(0, firstNewline);
            actualCode = codeSection.substr(firstNewline + 1);
        }
        if (!actualCode.empty() && actualCode.back() == '\n') actualCode.pop_back();

        // ==========================================
        // 5. RENDER THE MODERN CODE CARD
        // ==========================================
        ImGui::Spacing();
        ImDrawList *drawList = ImGui::GetWindowDrawList();

        // MAGIC TRICK: Split drawing into Background (Channel 0) and Foreground (Channel 1)
        drawList->ChannelsSplit(2);
        drawList->ChannelsSetCurrent(1); // Tell ImGui to draw text on the foreground layer

        // Capture the start coordinate of the code card
        ImVec2 blockStart = ImGui::GetCursorScreenPos();
        float availWidth = ImGui::GetContentRegionAvail().x;

        // Add internal padding for the text
        blockStart.x += 10.0f;
        blockStart.y += 10.0f;
        ImGui::SetCursorScreenPos(blockStart);

        ImGui::BeginGroup(); // Group the header and code together

        // --- HEADER: Language & Inner Copy Button ---
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::Text("%s", language.empty() ? "code" : language.c_str());
        ImGui::PopStyleColor();

        ImGui::SameLine(availWidth - 80.0f);
        if (ImGui::Button(("Copy##" + std::to_string(codeStart)).c_str())) {
            ImGui::SetClipboardText(actualCode.c_str());
        }
        ImGui::Separator();

        // --- CODE CONTENT ---
        ImGui::PushFont(jbFont); // Use your new monospace font!
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f)); // Crisp white text
        ImGui::TextWrapped("%s", actualCode.c_str());
        ImGui::PopStyleColor();
        ImGui::PopFont();

        ImGui::EndGroup(); // Finish measuring the text

        // Capture where the text ended
        ImVec2 blockEnd = ImGui::GetCursorScreenPos();
        blockEnd.y += 10.0f; // Add bottom padding

        // --- BACKGROUND RECTANGLE ---
        drawList->ChannelsSetCurrent(0); // Switch to the background layer
        drawList->AddRectFilled(
            ImVec2(blockStart.x - 10.0f, blockStart.y - 10.0f),
            ImVec2(blockStart.x - 10.0f + availWidth, blockEnd.y),
            IM_COL32(20, 22, 27, 255), // Deep, dark "IDE" background color
            8.0f // Rounded corners
        );

        drawList->ChannelsMerge(); // Combine the layers back together

        // Push the cursor below the newly drawn code block so the next text starts properly
        ImGui::SetCursorScreenPos(ImVec2(blockStart.x - 10.0f, blockEnd.y + 10.0f));

        pos = codeEnd + 3; // Move the parser past the closing ```
    }
}

bool GUI::Initialize(int windowWidth, int windowHeight, const std::string &windowTitle) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }

    const char *glsl_version = "#version 130";
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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImFontConfig fontConfig;
    fontConfig.OversampleH = 3;
    fontConfig.OversampleV = 3;
    GUI::fancy = io.Fonts->AddFontFromFileTTF("/home/metanika/CLionProjects/Branchat/Assets/PlayfairDisplay-VariableFont_wght.ttf", 20.0f, &fontConfig);
    GUI::jbFont = io.Fonts->AddFontFromFileTTF("/home/metanika/CLionProjects/Branchat/Assets/JetBrainsMono-ExtraLight.ttf", 20.0f, &fontConfig);
    GUI::roboto = io.Fonts->AddFontFromFileTTF("/home/metanika/CLionProjects/Branchat/Assets/Roboto-VariableFont_wdth,wght.ttf", 20.0f, &fontConfig);
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Dropping image files onto the window attaches them to the next message
    glfwSetDropCallback(window, OnFilesDropped);


    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 12.0f;
    style.GrabRounding = 6.0f;

    // Add air around elements
    style.FramePadding = ImVec2(8, 6);
    style.ItemSpacing = ImVec2(8, 8);
    style.ItemInnerSpacing = ImVec2(6, 6);

    ImGui::StyleColorsDark(); // Start with the dark preset

    // Tweak the accents to match your branding (e.g., a nice blue)
    style.Colors[ImGuiCol_Header] = ImVec4(0.2f, 0.4f, 0.8f, 0.6f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.2f, 0.5f, 0.9f, 0.8f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.2f, 0.2f, 0.25f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.3f, 0.35f, 1.0f);
    return true;
}

void GUI::StartRenderLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        CollectPickedImages();
        TextureCache::CollectGarbage();
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
    ImGui::PushFont(roboto);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
    ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
    ImGui::Begin("Branchat", nullptr,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
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
            ImGui::TextUnformatted((Session::globalErrorMessage).c_str());
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
            Action = [](const std::string &inputStr) { Base::AddGroup(inputStr); };
            triggerModal = true;
        }
        ImGui::Spacing(); // Adds a small visual gap

        // 2. Create the dynamic button label
        // This ensures the button always displays the current active limit
        std::string contextLabel = "Context Limit: " + std::to_string(Session::contextLimit);

        // 3. Draw the Context Limit button
        // ImVec2(ImGui::GetContentRegionAvail().x, 0) makes it span the full width of the side panel
        if (ImGui::Button(contextLabel.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            // Tell ImGui to prepare the popup dialog
            ImGui::OpenPopup("Edit Context Limit");
        }

        // 4. Define the Popup Modal logic
        // ImGuiWindowFlags_AlwaysAutoResize ensures the popup shrinks to fit the input field perfectly
        if (ImGui::BeginPopupModal("Edit Context Limit", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {

            ImGui::Spacing();

            // Input field to modify the variable
            ImGui::InputInt("##LimitInput", &contexLimitBuffer);

            ImGui::Spacing();

            // 5. Provide Save/Cancel buttons
            if (ImGui::Button("Save", ImVec2(120, 0))) {

                // Check if the input is outside the [1; 60] range
                if (contexLimitBuffer < 1 || contexLimitBuffer > 60) {
                    std::lock_guard<std::mutex> lock(Session::globalErrorMutex);
                    Session::globalErrorMessage = "Context limit out of range [1 ; 60]";
                    ImGui::CloseCurrentPopup();
                } else {
                    Session::contextLimit = contexLimitBuffer;
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::SameLine(0.0f, 20.0f);

            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }

            // Must be called to end the popup block
            ImGui::EndPopup();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        for (auto &group: Base::groups) {
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
            if (group.roots.empty()) groupFlags |= ImGuiTreeNodeFlags_Leaf;

            // 1. VISUAL FIX: Align text to button height to prevent vertical jitter
            ImGui::AlignTextToFramePadding();

            // Temporarily hide default ImGui harsh blue squares
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));

            std::string groupLabel = group.name + "###" + std::to_string(group.id);
            bool groupOpen = ImGui::TreeNodeEx(groupLabel.c_str(), groupFlags);

            ImGui::PopStyleColor(2);

            // 2. GET ITEM RECTANGLE FOR CUSTOM HOVER
            ImVec2 groupMin = ImGui::GetItemRectMin();
            ImVec2 groupMax = ImGui::GetItemRectMax();
            groupMax.x += 40.0f; // Expand hitbox slightly right to include the button area

            bool groupHovered = ImGui::IsMouseHoveringRect(groupMin, groupMax);
            bool groupPopupOpen = ImGui::IsPopupOpen("GroupOptions");

            // 3. DRAW CUSTOM PILL HIGHLIGHT
            if (groupHovered || groupPopupOpen) {
                // Draw the pill strictly around the text + button area
                ImGui::GetWindowDrawList()->AddRectFilled(groupMin, groupMax, IM_COL32(255, 255, 255, 15), 15.0f);
            }

            // 4. VISUAL FIX: Keep button close to name, but use Alpha to hide it
            ImGui::SameLine(0.0f, 15.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, (groupHovered || groupPopupOpen) ? 1.0f : 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 15.0f);
            if (ImGui::Button("...##grpBtn")) ImGui::OpenPopup("GroupOptions");
            ImGui::PopStyleVar(2);

            // --- POPUP LOGIC REMAINS EXACTLY THE SAME ---
            if (ImGui::BeginPopup("GroupOptions")) {
                if (ImGui::Selectable("Add Root")) {
                    forceOpenGroupId = group.id;
                    Action = [&group](const std::string &inputStr) {
                        group.AddRoot(inputStr);
                        forceOpenGroupId = group.id;
                    };
                    triggerModal = true;
                }
                if (ImGui::Selectable("Rename")) {
                    Action = [&group](const std::string &inputStr) { group.Rename(inputStr); };
                    triggerModal = true;
                }
                if (ImGui::Selectable("Delete")) {
                    Action = [&group](std::string ff) { group.Delete(); };
                    pendingAction = true;
                }
                ImGui::EndPopup();
            }

            if (groupOpen) {
                for (auto &root: group.roots) {
                    ImGui::PushID(root.id);

                    bool isExpanded = (Tree::rootId == root.id);
                    bool isSelected = (Session::activeChatId == root.id);

                    ImGui::SetNextItemOpen(isExpanded, ImGuiCond_Always);

                    ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_OpenOnArrow;
                    if (isSelected) rootFlags |= ImGuiTreeNodeFlags_Selected;
                    if (!root.hasChildren) rootFlags |= ImGuiTreeNodeFlags_Leaf;

                    // VISUAL FIX: Align text for the root nodes too
                    ImGui::AlignTextToFramePadding();

                    // Hide default ImGui selection colors
                    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));

                    std::string rootLabel = root.name + "###" + std::to_string(root.id);
                    bool rootOpen = ImGui::TreeNodeEx(rootLabel.c_str(), rootFlags);

                    ImGui::PopStyleColor(3);

                    // --- INTERACTION LOGIC REMAINS EXACTLY THE SAME ---
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
                    } else if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                        root.FocusChat();
                    }

                    // HOVER LOGIC: ROOT CHAT
                    ImVec2 rootMin = ImGui::GetItemRectMin();
                    ImVec2 rootMax = ImGui::GetItemRectMax();

                    // We add enough space to the hitbox to cover the padding + the button width
                    rootMax.x += 45.0f;

                    bool rootHovered = ImGui::IsMouseHoveringRect(rootMin, rootMax);
                    bool rootPopupOpen = ImGui::IsPopupOpen("RootOptions");

                    // Custom Background for Roots (Brighter if actively selected!)
                    if (isSelected || rootHovered || rootPopupOpen) {
                        ImU32 pillColor = isSelected ? IM_COL32(255, 255, 255, 30) : IM_COL32(255, 255, 255, 15);
                        ImGui::GetWindowDrawList()->AddRectFilled(rootMin, rootMax, pillColor, 15.0f);
                    }

                    // ==========================================
                    // VISUAL FIX: Padding, Alpha, and Rounding
                    // ==========================================
                    ImGui::SameLine(0.0f, 15.0f); // 15px of padding between the name and the dots

                    // Push BOTH the transparency and the rounded corners
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, (rootHovered || rootPopupOpen) ? 1.0f : 0.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 15.0f);

                    if (ImGui::Button("...##rtBtn")) ImGui::OpenPopup("RootOptions");

                    ImGui::PopStyleVar(2); // Pop BOTH variables
                    // ==========================================

                    // --- POPUP LOGIC REMAINS EXACTLY THE SAME ---
                    if (ImGui::BeginPopup("RootOptions")) {
                        if (ImGui::Selectable("Create Branch")) {
                            Action = [&root](const std::string &inputStr) { root.CreateBranch(inputStr); };
                            triggerModal = true;
                        }
                        if (ImGui::Selectable("Rename")) {
                            Action = [&root](const std::string &inputStr) { root.Rename(inputStr); };
                            triggerModal = true;
                        }
                        if (ImGui::Selectable("Delete")) {
                            Action = [&root](const std::string &inputStr) { root.Delete(); };
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
        ImGui::PopFont();
        ImGui::PopStyleVar();
        ImGui::EndChild();

        // ==========================================
        // RIGHT PANEL: CHAT INTERFACE
        // ==========================================
        ImGui::TableSetColumnIndex(1);
        ImGui::BeginChild("RightPanel", ImVec2(0, 0), false);

     if (Session::chatPtr != nullptr) {
    // 1. HEADER
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Active Chat: %s", Session::chatPtr->name.c_str());
    ImGui::Separator();

    // --- [MODIFIED]: DYNAMIC LAYOUT CALCULATION ---
    float sendButtonWidth = ImGui::GetFrameHeightWithSpacing() * 1.5f;
    float attachButtonWidth = ImGui::CalcTextSize("Attach").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    float inputAreaWidth = ImGui::GetContentRegionAvail().x - sendButtonWidth - attachButtonWidth
                           - ImGui::GetStyle().ItemSpacing.x * 2.0f;

    // Calculate prompt bar height
    ImVec2 textSize = ImGui::CalcTextSize(chatInputBuffer, NULL, false, inputAreaWidth);
    float promptBarHeight = std::clamp(textSize.y + (ImGui::GetStyle().FramePadding.y * 4.0f),
                                       ImGui::GetTextLineHeightWithSpacing() * 1.5f,
                                       ImGui::GetWindowHeight() * 0.35f);

    // Height of the static control area (Buttons + Nav bar + Spacing)
    float staticControlsHeight = 85.0f;
    float totalFooterHeight = promptBarHeight + staticControlsHeight + PendingAttachmentsHeight();

    // 2. MESSAGE HISTORY (Yields space to the footer)
    ImGui::BeginChild("MessageScroll", ImVec2(0, -totalFooterHeight), true);
    float maxWrapWidth = ImGui::GetWindowWidth() * 0.75f;

    for (const auto &msg: Session::chatPtr->messages) {
        ImGui::PushID(&msg);
        ImVec2 rowStartScreenPos = ImGui::GetCursorScreenPos();
        float availWidth = ImGui::GetContentRegionAvail().x;
        float screenStartX = rowStartScreenPos.x;

        bool isUser = (msg.Role == "user");
        if (!msg.Attachments.empty()) {
            RenderMessageAttachments(msg.Attachments, isUser);
        }

        const bool hasText = !msg.Content.empty();
        if (hasText) {
            if (isUser) {
                ImVec2 tSize = ImGui::CalcTextSize(msg.Content.c_str(), NULL, false, maxWrapWidth);
                float textLocalStartX = availWidth - tSize.x - ImGui::GetStyle().ItemSpacing.x;
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(textLocalStartX, 0.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 1.0f, 1.0f));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));
                ImGui::PushFont(GUI::roboto);
            }

            RenderMessageContent(msg.Content, isUser);
            if (!isUser) ImGui::PopFont();
            ImGui::PopStyleColor();
        }

        float footerH = 30.0f;
        ImVec2 fStart = ImGui::GetCursorScreenPos();
        ImVec2 fEnd = ImVec2(screenStartX + availWidth, fStart.y + footerH);
        ImVec2 labelSize = ImGui::CalcTextSize("Copy");
        ImVec2 btnPadding = ImVec2(12.0f, 4.0f);
        ImVec2 btnSize = ImVec2(labelSize.x + btnPadding.x * 2.0f, labelSize.y + btnPadding.y * 2.0f);
        float vOffset = (footerH - btnSize.y) / 2.0f;

        bool rHovered = ImGui::IsMouseHoveringRect(rowStartScreenPos, fEnd);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, btnPadding);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, rHovered ? 1.0f : 0.0f);
        float btnX = isUser ? (screenStartX + availWidth - btnSize.x - 10.0f) : (screenStartX + 10.0f);
        ImGui::SetCursorScreenPos(ImVec2(btnX, fStart.y + vOffset));
        if (hasText && ImGui::Button("Copy", btnSize)) ImGui::SetClipboardText(msg.Content.c_str());
        ImGui::PopStyleVar(3);
        ImGui::SetCursorScreenPos(ImVec2(screenStartX, fStart.y + footerH));
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::PopID();
    }
         if (scrollBottom) {
             ImGui::SetScrollHereY(0.0f);
             scrollBottom = false;
         }
         if (scrollDown) {
             ImGui::SetScrollY(ImGui::GetScrollY() + 220.0f);
             scrollDown = false;
         }
    ImGui::EndChild();

    // 3. PROMPT BAR (Static Height)
    ImGui::Separator();
    ImGui::PushFont(roboto);
    if (Session::chatPtr->waitingResponse) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "AI is responding...");
    } else {
        ImGui::Spacing();
    }

    bool isWaiting = Session::chatPtr->waitingResponse;
    if (isWaiting) ImGui::BeginDisabled();

    // Images waiting to be sent with the next message
    RenderPendingAttachments();

    // [MODIFIED]: WordWrap + Multiline, height based on calculated promptBarHeight
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 0.0f);
    ImGui::InputTextMultiline("##ChatInput", chatInputBuffer, IM_ARRAYSIZE(chatInputBuffer),
                              ImVec2(inputAreaWidth, promptBarHeight),
                              ImGuiInputTextFlags_CtrlEnterForNewLine | ImGuiInputTextFlags_WordWrap);
    ImGui::PopStyleVar();

    ImGui::SameLine();
    const bool dialogOpen = ImageInput::IsFileDialogOpen();
    if (dialogOpen) ImGui::BeginDisabled();
    if (ImGui::Button("Attach", ImVec2(attachButtonWidth, promptBarHeight))) {
        ImageInput::OpenFileDialogAsync();
    }
    if (dialogOpen) ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Attach images\n(you can also paste with Ctrl+V or drag files here)");
    }

    // Ignore keyboard shortcuts while a dialog (rename, image viewer, ...) is open
    const bool popupOpen = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);

    ImGui::SameLine();
    bool sendPressed = ImGui::Button("Send", ImVec2(-1, promptBarHeight));
    sendPressed |= !popupOpen && ImGui::IsKeyPressed(ImGuiKey_Enter);
    if (sendPressed && !isWaiting && (chatInputBuffer[0] != '\0' || !pendingAttachments.empty())) {
        const size_t countBefore = Session::chatPtr->messages.size();
        Session::chatPtr->SendPrompt(std::string(chatInputBuffer), pendingAttachments);
        // Only clear the input if the message was actually saved
        if (Session::chatPtr->messages.size() > countBefore) {
            chatInputBuffer[0] = '\0';
            pendingAttachments.clear();
            scrollBottom = true;
        }
    }

    // Ctrl+V with an image in the clipboard attaches it
    if (!isWaiting && !popupOpen && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V, false)) {
        PasteImageFromClipboard();
    }
    if (isWaiting) ImGui::EndDisabled();

    // 4. BOTTOM ACTION BAR (Static, always visible)
    ImGui::Spacing();
    float dotsWidth = 40.0f;
    float navBtnWidth = 120.0f;
    float paddingRight = ImGui::GetStyle().WindowPadding.x;
    bool showNavButtons = !Session::pointingAtRoot;

    if (showNavButtons) {
        if (ImGui::Button("^ Parent", ImVec2(navBtnWidth, 0))) {
            auto tempChatPtr = static_cast<Chat *>(Session::chatPtr);
            int parentId = tempChatPtr->parentId;
            auto parentNode = Tree::FindChatById(Tree::topBranches, parentId);
            if (parentNode) parentNode->FocusChat();
            else { Base::FindGroup(Session::activeGroupId)->FindRootChat(parentId)->FocusChat(); }
        }
        ImGui::SameLine();
        if (ImGui::Button("^^ Root", ImVec2(navBtnWidth, 0))) {
            Base::FindGroup(Session::activeGroupId)->FindRootChat(Tree::rootId)->FocusChat();
        }
    }

    ImGui::PopFont();
    if (showNavButtons) ImGui::SameLine(ImGui::GetWindowWidth() - dotsWidth - paddingRight);
    else ImGui::SetCursorPosX(ImGui::GetWindowWidth() - dotsWidth - paddingRight);

    if (ImGui::Button("...##ActiveChatDots", ImVec2(dotsWidth, 0))) ImGui::OpenPopup("ActiveChatOptionsPopup");

    // Options Popup
    if (ImGui::BeginPopup("ActiveChatOptionsPopup")) {
        if (ImGui::Selectable("Create Branch")) { triggerModal = true; Action = [](const std::string &s) { Session::chatPtr->CreateBranch(s); }; }
        if (ImGui::Selectable("Rename")) { strncpy(modalInputBuffer, Session::chatPtr->name.c_str(), sizeof(modalInputBuffer) - 1); triggerModal = true; Action = [](const std::string &s) { Session::chatPtr->Rename(s); }; }
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Selectable("Delete")) { pendingAction = true; Action = [](const std::string &s) { Session::chatPtr->Delete(); }; }
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

void GUI::RenderTreeNodes(std::vector<Chat> &branches) {
    for (auto &chat: branches) {
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

        // 1. VISUAL FIX: Align text height to match the button height
        ImGui::AlignTextToFramePadding();

        // 2. Hide default ImGui selection colors so we can draw our custom pill
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));

        std::string chatLabel = chat.name + "###" + std::to_string(chat.id);
        bool nodeOpen = ImGui::TreeNodeEx(chatLabel.c_str(), flags);

        ImGui::PopStyleColor(3);

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            chat.FocusChat();
        }

        // ==========================================
        // HOVER LOGIC & CUSTOM HIGHLIGHT
        // ==========================================
        ImVec2 chatMin = ImGui::GetItemRectMin();
        ImVec2 chatMax = ImGui::GetItemRectMax();
        chatMax.x += 45.0f; // Stretch hover zone to cover the padding + button width

        bool chatHovered = ImGui::IsMouseHoveringRect(chatMin, chatMax);
        bool chatPopupOpen = ImGui::IsPopupOpen("ChatOptions");

        // Custom Background (Brighter if actively selected!)
        if (isSelected || chatHovered || chatPopupOpen) {
            ImU32 pillColor = isSelected ? IM_COL32(255, 255, 255, 30) : IM_COL32(255, 255, 255, 15);

            // 3. VISUAL FIX: Background pill with 15.0f rounding
            ImGui::GetWindowDrawList()->AddRectFilled(chatMin, chatMax, pillColor, 15.0f);
        }

        // ==========================================
        // THE THREE DOTS BUTTON
        // ==========================================
        ImGui::SameLine(0.0f, 15.0f); // 15px of padding between name and dots

        // Push Alpha (Visibility) and Rounding (15.0f)
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, (chatHovered || chatPopupOpen) ? 1.0f : 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 15.0f); // 4. VISUAL FIX: Button rounding

        if (ImGui::Button("...##chatBtn")) ImGui::OpenPopup("ChatOptions");

        ImGui::PopStyleVar(2); // Pop both Alpha and Rounding

        // ==========================================
        // POPUP LOGIC
        // ==========================================
        if (ImGui::BeginPopup("ChatOptions")) {
            if (ImGui::Selectable("Create Branch")) {
                triggerModal = true;
                Action = [&chat](const std::string &inputStr) {
                    chat.CreateBranch(inputStr);
                    forceOpenChatId = chat.id;
                };
            }
            if (ImGui::Selectable("Rename")) {
                strncpy(modalInputBuffer, chat.name.c_str(), sizeof(modalInputBuffer) - 1);
                triggerModal = true;
                Action = [&chat](const std::string &inputStr) { chat.Rename(inputStr); };
            }
            if (ImGui::Selectable("Delete")) {
                Action = [&chat](const std::string &inputStr) { chat.Delete(); };
                pendingAction = true;
            }
            ImGui::EndPopup();
        }

        // ==========================================
        // RECURSIVE RENDERING
        // ==========================================
        if (nodeOpen) {
            RenderTreeNodes(chat.branches);
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}

void GUI::RenderModals() {
    RenderImageViewer();
    if (triggerModal) {
        // std::cout << "DEBUG: Trigger detected! Attempting to open popup." << std::endl;
        ImGui::OpenPopup("##UniqueInputModal");
        triggerModal = false; // Reset the flag immediately
    }
    ImGui::PushFont(roboto);
    if (ImGui::BeginPopupModal("##UniqueInputModal", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter name:");
        if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();

        bool enterPressed = ImGui::InputText("##NameInput", modalInputBuffer, IM_ARRAYSIZE(modalInputBuffer),
                                             ImGuiInputTextFlags_EnterReturnsTrue);

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
    ImGui::PopFont();
}

void GUI::ExecuteDeferredActions() {
    if (pendingAction) {
        Action(std::string(modalInputBuffer));
        pendingAction = false;
        Action = nullptr;
        modalInputBuffer[0] = '\0';
    }
}

// ==========================================
// IMAGES
// ==========================================
namespace {
    constexpr size_t MaxPendingAttachments = 10;
    constexpr float ThumbnailHeight = 64.0f;

    // Largest size that keeps the image's aspect ratio and fits in maxW x maxH (never upscales)
    ImVec2 FitImage(const Attachment &attachment, float maxW, float maxH) {
        if (const ImageTexture *tex = TextureCache::Get(attachment.data)) {
            const float scale = std::min({1.0f, maxW / tex->width, maxH / tex->height});
            return ImVec2(tex->width * scale, tex->height * scale);
        }
        return ImVec2(std::min(maxW, 220.0f), std::min(maxH, 64.0f)); // placeholder box
    }

    std::string DescribeAttachment(const Attachment &attachment) {
        const double kb = attachment.Size() / 1024.0;
        char size[32];
        if (kb >= 1024.0) snprintf(size, sizeof(size), "%.1f MB", kb / 1024.0);
        else snprintf(size, sizeof(size), "%.0f KB", kb);
        return (attachment.fileName.empty() ? std::string("image") : attachment.fileName) + " (" + size + ")";
    }
}

void GUI::AddPendingAttachment(Attachment attachment) {
    if (Session::chatPtr == nullptr) {
        Session::SetError("Open a chat before adding images.");
        return;
    }
    if (pendingAttachments.size() >= MaxPendingAttachments) {
        Session::SetError("You can attach up to " + std::to_string(MaxPendingAttachments) + " images per message.");
        return;
    }
    size_t total = attachment.Size();
    for (const auto &a: pendingAttachments) total += a.Size();
    if (total > AIAPI::ImageBudgetBytes) {
        Session::SetError("These images are too large to send together (limit " +
                          std::to_string(AIAPI::ImageBudgetBytes / (1024 * 1024)) + " MB per message).");
        return;
    }
    pendingAttachments.push_back(std::move(attachment));
}

void GUI::PasteImageFromClipboard() {
    // Text in the clipboard is a normal text paste, which the input box handles
    const char *text = glfwGetClipboardString(window);
    if (text != nullptr && text[0] != '\0') return;

    try {
        if (auto image = ImageInput::ReadClipboardImage()) {
            AddPendingAttachment(std::move(*image));
        }
    } catch (const std::exception &e) {
        Session::SetError(e.what());
    }
}

void GUI::CollectPickedImages() {
    std::vector<Attachment> images;
    std::vector<std::string> errors;
    ImageInput::TakeFileDialogResults(images, errors);
    for (auto &image: images) AddPendingAttachment(std::move(image));
    if (!errors.empty()) {
        std::string message;
        for (const auto &err: errors) message += (message.empty() ? "" : "\n") + err;
        Session::SetError(message);
    }
}

void GUI::OnFilesDropped(GLFWwindow *, int count, const char **paths) {
    std::string errors;
    for (int i = 0; i < count; ++i) {
        try {
            AddPendingAttachment(Attachment::FromFile(paths[i]));
        } catch (const std::exception &e) {
            errors += (errors.empty() ? "" : "\n") + std::string(e.what());
        }
    }
    if (!errors.empty()) Session::SetError(errors);
}

bool GUI::DrawImage(const Attachment &attachment, ImVec2 size) {
    const bool clicked = ImGui::InvisibleButton("##image", size);
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    ImDrawList *drawList = ImGui::GetWindowDrawList();

    if (const ImageTexture *tex = TextureCache::Get(attachment.data)) {
        drawList->AddImageRounded(static_cast<ImTextureID>(tex->id), min, max,
                                  ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, 8.0f);
    } else {
        // Format stb can't decode (e.g. WEBP): the AI still gets it, we just can't preview it
        drawList->AddRectFilled(min, max, IM_COL32(40, 42, 50, 255), 8.0f);
        const std::string label = "[no preview] " + attachment.fileName;
        drawList->PushClipRect(min, max, true);
        drawList->AddText(ImVec2(min.x + 8.0f, min.y + 8.0f), IM_COL32(180, 180, 180, 255), label.c_str());
        drawList->PopClipRect();
    }
    if (ImGui::IsItemHovered()) {
        drawList->AddRect(min, max, IM_COL32(255, 255, 255, 80), 8.0f, 0, 1.5f);
    }
    return clicked;
}

void GUI::RenderMessageAttachments(const std::vector<Attachment> &attachments, bool alignRight) {
    const float avail = ImGui::GetContentRegionAvail().x;
    const float maxW = std::min(avail * 0.6f, 420.0f);
    const float maxH = 320.0f;

    for (const auto &attachment: attachments) {
        ImGui::PushID(&attachment);
        const ImVec2 size = FitImage(attachment, maxW, maxH);
        if (alignRight) {
            const float offset = avail - size.x - ImGui::GetStyle().ItemSpacing.x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(offset, 0.0f));
        }
        if (DrawImage(attachment, size)) {
            viewerImage = attachment.data;
            triggerViewer = true;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            ImGui::SetTooltip("%s\nClick to enlarge", DescribeAttachment(attachment).c_str());
        }
        ImGui::PopID();
    }
}

float GUI::PendingAttachmentsHeight() {
    return pendingAttachments.empty() ? 0.0f : ThumbnailHeight + ImGui::GetStyle().ItemSpacing.y;
}

void GUI::RenderPendingAttachments() {
    if (pendingAttachments.empty()) return;

    int removeIndex = -1;
    for (size_t i = 0; i < pendingAttachments.size(); ++i) {
        const Attachment &attachment = pendingAttachments[i];
        ImGui::PushID(static_cast<int>(i));
        if (i > 0) ImGui::SameLine();

        const ImVec2 size = FitImage(attachment, ThumbnailHeight * 1.6f, ThumbnailHeight);
        if (DrawImage(attachment, size)) removeIndex = static_cast<int>(i);

        if (ImGui::IsItemHovered()) {
            // Darken the thumbnail and show what a click does
            const ImVec2 min = ImGui::GetItemRectMin();
            const ImVec2 max = ImGui::GetItemRectMax();
            ImDrawList *drawList = ImGui::GetWindowDrawList();
            drawList->AddRectFilled(min, max, IM_COL32(0, 0, 0, 150), 8.0f);
            const ImVec2 textSize = ImGui::CalcTextSize("Remove");
            drawList->AddText(ImVec2((min.x + max.x - textSize.x) * 0.5f, (min.y + max.y - textSize.y) * 0.5f),
                              IM_COL32(255, 120, 120, 255), "Remove");
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            ImGui::SetTooltip("%s", DescribeAttachment(attachment).c_str());
        }
        ImGui::PopID();
    }

    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (ThumbnailHeight - ImGui::GetTextLineHeight()) * 0.5f);
    ImGui::TextDisabled("%zu image(s) attached", pendingAttachments.size());

    if (removeIndex >= 0) {
        pendingAttachments.erase(pendingAttachments.begin() + removeIndex);
    }
}

void GUI::RenderImageViewer() {
    constexpr const char *popupId = "Image##Viewer";
    if (triggerViewer) {
        ImGui::OpenPopup(popupId);
        triggerViewer = false;
    }

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    bool open = true;
    if (ImGui::BeginPopupModal(popupId, &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
        bool close = ImGui::IsKeyPressed(ImGuiKey_Escape);
        if (const ImageTexture *tex = TextureCache::Get(viewerImage)) {
            const float scale = std::min({1.0f, viewport->Size.x * 0.85f / tex->width,
                                          viewport->Size.y * 0.8f / tex->height});
            ImGui::Image(static_cast<ImTextureID>(tex->id), ImVec2(tex->width * scale, tex->height * scale));
            if (ImGui::IsItemClicked()) close = true;
            ImGui::TextDisabled("%d x %d  -  click or press Esc to close", tex->width, tex->height);
        } else {
            ImGui::TextUnformatted("No preview available for this image format.");
        }
        if (close) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    if (!ImGui::IsPopupOpen(popupId)) viewerImage.reset();
}
