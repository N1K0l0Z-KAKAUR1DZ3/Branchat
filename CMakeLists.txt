cmake_minimum_required(VERSION 4.2)
project(Branchat1)

set(CMAKE_CXX_STANDARD 20)

add_executable(Branchat1 main.cpp Services/Sources/Session.cpp APIs/Sources/DBAPI.cpp Dependencies/nanodbc/nanodbc.cpp
        Models/Sources/Group.cpp
        Models/Sources/Root_Chat.cpp
        Models/Sources/Chat.cpp
        Models/Sources/Message.cpp
        Models/Sources/Base.cpp
        Models/Sources/Tree.cpp
        Models/Headers/Base.h
        Models/Headers/Chat.h
        Models/Headers/Group.h
        Models/Headers/Message.h
        Models/Headers/Root_chat.h
        Models/Headers/Tree.h
        ../../../../usr/local/include/DBAPI.h
        ../../../../usr/local/include/Session.h
        APIs/Headers/DBAPI.h
        Services/Headers/Session.h
        APIs/Headers/AIAPI.h
        APIs/Sources/AIAPI.cpp
)

target_link_libraries(Branchat1 odbc)