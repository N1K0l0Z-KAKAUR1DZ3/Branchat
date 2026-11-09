cmake_minimum_required(VERSION 3.24)
project(Branchat1 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch nlohmann_json
FetchContent_Declare(nlohmann_json GIT_REPOSITORY https://github.com/nlohmann/json.git GIT_TAG v3.11.3)
FetchContent_MakeAvailable(nlohmann_json)

# Fetch Dear ImGui
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG docking)
FetchContent_MakeAvailable(imgui)

# Fetch stb (header-only image decode/encode, used for image attachments)
FetchContent_Declare(stb GIT_REPOSITORY https://github.com/nothings/stb.git GIT_TAG 4e9b368bfc071e97206ea5c6adba68dd0cbf8897)
FetchContent_MakeAvailable(stb)

find_package(CURL REQUIRED)
find_package(ODBC REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

set(PROJECT_SOURCES
        main.cpp
        Session/Session.cpp
        APIs/Sources/DBAPI.cpp
        Dependencies/nanodbc/nanodbc.cpp
        Models/Sources/Group.cpp
        Models/Sources/Root_Chat.cpp
        Models/Sources/Chat.cpp
        Models/Sources/Message.cpp
        Models/Sources/Attachment.cpp
        Models/Sources/Base.cpp
        Models/Sources/Tree.cpp
        APIs/Sources/AIAPI.cpp
        GUI/GUI.cpp
        GUI/ImageInput.cpp
        GUI/TextureCache.cpp
)

set(IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

set(ADDONS_DIR "Dependencies/imgui")
set(ADDON_SOURCES
        ${ADDONS_DIR}/ImGuiColorTextEdit-master/TextEditor.cpp
)

add_executable(Branchat1 ${PROJECT_SOURCES} ${IMGUI_SOURCES} ${ADDON_SOURCES})

# Required definitions for nanodbc + unixODBC without UTF-16 string conversion
target_compile_definitions(Branchat1 PRIVATE
        NANODBC_USE_ODBC_VERSION=0x0380
        SQL_WCHART_CONVERT
        # Absolute path to the Assets folder, so fonts load regardless of user or CWD
        BRANCHAT_ASSETS_DIR="${CMAKE_SOURCE_DIR}/Assets"
)

target_include_directories(Branchat1 PRIVATE
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${stb_SOURCE_DIR}
        Models/Headers
        APIs/Headers
        Session
        Dependencies
        ${ODBC_INCLUDE_DIRS}
        ${ADDONS_DIR}/ImGuiColorTextEdit-master
        ${ADDONS_DIR}/imgui_markdown-main
        ${ADDONS_DIR}/IconFontCppHeaders-main
)

target_link_libraries(Branchat1 PRIVATE
        nlohmann_json::nlohmann_json
        CURL::libcurl
        ODBC::ODBC
        OpenGL::GL
        glfw
)