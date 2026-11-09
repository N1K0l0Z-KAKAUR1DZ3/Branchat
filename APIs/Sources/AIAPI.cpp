#include "../Headers/AIAPI.h"
#include <curl/curl.h>

void AIAPI::init() {
    std::cout <<"Initializing AI API..." << std::endl;
    url = std::getenv("API_URL1");
    CURLcode globalRes = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (globalRes != CURLE_OK) {
        throw std::runtime_error("CRITICAL: Failed to initialize libcurl globally: " + std::string(curl_easy_strerror(globalRes)));
    }
    std::cout<<"initialized AI API"<<std::endl;
}

std::string AIAPI::ParseRawJsonResponce(std::string &rawJsonResponse) {
    nlohmann::json parsedData = nlohmann::json::parse(rawJsonResponse);
    if (parsedData.contains("error")) {
        throw std::runtime_error(parsedData["error"]["message"]);
    }
    if (parsedData.contains("candidates") && !parsedData["candidates"].empty()) {

        auto& firstCandidate = parsedData["candidates"][0];
        if (firstCandidate.contains("content") && firstCandidate["content"].contains("parts")) {

            auto& partsArray = firstCandidate["content"]["parts"];
            if (!partsArray.empty() && partsArray[0].contains("text")) {

                // Extract and return the clean string value!
                return partsArray[0]["text"].get<std::string>();
            }
        }
    }
    throw std::runtime_error("ERROR: Response was received, but text payload structure was invalid or empty.");
}

std::string AIAPI::Base64Encode(const std::vector<std::uint8_t>& bytes) {
    static constexpr char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((bytes.size() + 2) / 3) * 4);
    size_t i = 0;
    for (; i + 2 < bytes.size(); i += 3) {
        const uint32_t n = (bytes[i] << 16) | (bytes[i + 1] << 8) | bytes[i + 2];
        out.push_back(table[(n >> 18) & 63]);
        out.push_back(table[(n >> 12) & 63]);
        out.push_back(table[(n >> 6) & 63]);
        out.push_back(table[n & 63]);
    }
    if (i < bytes.size()) {
        uint32_t n = bytes[i] << 16;
        if (i + 1 < bytes.size()) n |= bytes[i + 1] << 8;
        out.push_back(table[(n >> 18) & 63]);
        out.push_back(table[(n >> 12) & 63]);
        out.push_back(i + 1 < bytes.size() ? table[(n >> 6) & 63] : '=');
        out.push_back('=');
    }
    return out;
}

std::string AIAPI::SerializeJSONPayload(const std::vector<Message> &messageHistory) {
    // Decide which images fit in the request, newest first, so the latest
    // screenshots always get through and old ones are dropped first.
    std::vector<bool> includeImages(messageHistory.size(), false);
    size_t imageBytes = 0;
    for (size_t i = messageHistory.size(); i-- > 0;) {
        size_t msgBytes = 0;
        for (const auto& att : messageHistory[i].Attachments) msgBytes += att.Size();
        if (msgBytes == 0) continue;
        if (imageBytes + msgBytes <= ImageBudgetBytes) {
            includeImages[i] = true;
            imageBytes += msgBytes;
        }
    }

    nlohmann::json rootPayload =  nlohmann::json::object();
    rootPayload["contents"] =  nlohmann::json::array();
    for (size_t i = 0; i < messageHistory.size(); ++i) {
        const auto& msg = messageHistory[i];

        nlohmann::json parts = nlohmann::json::array();
        size_t omitted = 0;
        for (const auto& att : msg.Attachments) {
            if (!att.data) continue;
            if (!includeImages[i]) {
                ++omitted;
                continue;
            }
            parts.push_back({
                {"inline_data", {
                    {"mime_type", att.mimeType},
                    {"data", Base64Encode(*att.data)}
                }}
            });
        }

        std::string text = msg.Content;
        if (omitted > 0) {
            text += "\n[" + std::to_string(omitted) + " earlier image(s) omitted to fit the request size limit]";
        }
        // Gemini needs at least one part per message
        if (!text.empty() || parts.empty()) {
            parts.push_back({{"text", text}});
        }

        nlohmann::json messageNode = nlohmann::json::object();
        messageNode["role"] = msg.Role;
        messageNode["parts"] = std::move(parts);
        rootPayload["contents"].push_back(std::move(messageNode));
    }
    if (imageBytes > 0) {
        std::cout << "[AIAPI] Sending " << imageBytes / 1024 << " KB of image data" << std::endl;
    }
    return rootPayload.dump();
}

size_t AIAPI::WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp) {
    size_t totalSize = size * nmemb;
    userp->append((char*)contents, totalSize);
    return totalSize;
}

std::string AIAPI::GetAIResponse(const std::vector<Message> &contextPayload) {
    const std::string dumpedJson = SerializeJSONPayload(contextPayload);
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("ERROR: Failed to allocate local cURL transmission handles.");
    }
    std::string responseBuffer;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, dumpedJson.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::string err = "Network Error: " + std::string(curl_easy_strerror(res));
        throw std::runtime_error(err);
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return ParseRawJsonResponce(responseBuffer);
}
AIAPI::~AIAPI() {
    curl_global_cleanup();
}