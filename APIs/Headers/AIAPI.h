#pragma once
#include "../../Dependencies/json.hpp"
#include "../../Models/Headers/Message.h"

#ifndef BRANCHAT1_AIAPI_H
#define BRANCHAT1_AIAPI_H
class AIAPI {
    static std::string ParseRawJsonResponce(std::string& json);
    static std::string SerializeJSONPayload(const std::vector<Message>& messageHistory);
    static std::string Base64Encode(const std::vector<std::uint8_t>& bytes);
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    inline static std::string url;
public:
    // Raw image bytes allowed in one request. Gemini rejects requests over 20 MB,
    // and base64 makes images ~33% bigger, so older images past this are left out.
    static constexpr std::size_t ImageBudgetBytes = 12 * 1024 * 1024;
    static void init();
    static std::string GetAIResponse(const std::vector<Message>& contextPayload);
    ~AIAPI();
};
#endif //BRANCHAT1_AIAPI_H
