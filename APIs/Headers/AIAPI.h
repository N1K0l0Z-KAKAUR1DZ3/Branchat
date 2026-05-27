#pragma once
#include "../../Dependencies/json.hpp"
#include "../../Models/Headers/Message.h"

#ifndef BRANCHAT1_AIAPI_H
#define BRANCHAT1_AIAPI_H
class AIAPI {
    static std::string ParseRawJsonResponce(std::string& json);
    static std::string SerializeJSONPayload(const std::vector<Message>& messageHistory);
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    inline static std::string url;
public:
    static void init();
    static std::string GetAIResponse(const std::vector<Message>& contextPayload);
    ~AIAPI();
};
#endif //BRANCHAT1_AIAPI_H
