#pragma once
#include "../APIs/Headers/DBAPI.h"
#include "../APIs/Headers/AIAPI.h"
#include <mutex>
#include <vector>

#ifndef BRANCHAT1_SESSION_H
#define BRANCHAT1_SESSION_H

// An AI reply produced on a worker thread, waiting to be applied on the UI thread.
struct PendingAIResult {
    int chatId;        // chat the reply belongs to
    int groupId;       // its group, for re-locating the chat
    int rootIdForMsg;  // Root_ID to store on the reply message
    bool ok;           // false => text holds an error message
    std::string text;
};

class Session {
    static std::vector<Message>* FetchAdditionalContext();

    // AI replies finished by worker threads; applied on the UI thread each frame.
    inline static std::mutex resultMutex;
    inline static std::vector<PendingAIResult> pendingResults;
public:
    inline static int contextLimit = 15;
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

    // Build the context to send, on the UI thread (returns a self-contained copy).
    static std::vector<Message> BuildContextPayload();
    // Run the network request on a worker thread using only its own copy of the data.
    static void RunAIRequest(std::vector<Message> payload, int chatId, int groupId, int rootIdForMsg);
    // Apply any finished replies to their chats; call once per frame on the UI thread.
    static void ApplyPendingResults();
};
#endif //BRANCHAT1_SESSION_H
