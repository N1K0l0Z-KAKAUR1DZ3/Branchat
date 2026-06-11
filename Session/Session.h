#pragma once
#include "../APIs/Headers/DBAPI.h"
#include "../APIs/Headers/AIAPI.h"

#ifndef BRANCHAT1_SESSION_H
#define BRANCHAT1_SESSION_H
class Session {
    static std::vector<Message>* FetchAdditionalContext();
public:
    inline static int contextLimit;
    inline static int activeChatId = -1;
    inline static int activeGroupId = -1;

    inline static bool pointingAtRoot = false;
    inline static RootChat* chatPtr;

    inline static std::string globalErrorMessage;
    inline static std::mutex globalErrorMutex;

    static void SetError(const std::string& err);
    Session();
    static void AttemptRehook();
    static void ReloadBase();
    static void ReloadTree();

    static void resetFocus();
    static std::string ReceiveAIResponse();
};
#endif //BRANCHAT1_SESSION_H
