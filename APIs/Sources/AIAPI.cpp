#include "../Headers/AIAPI.h"
#include <curl/curl.h>

void AIAPI::init() {
    url = std::getenv("API_URL");
    CURLcode globalRes = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (globalRes != CURLE_OK) {
        throw std::runtime_error("CRITICAL: Failed to initialize libcurl globally: " + std::string(curl_easy_strerror(globalRes)));
    }
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

std::string AIAPI::SerializeJSONPayload(const std::vector<Message> &messageHistory) {
    nlohmann::json rootPayload =  nlohmann::json::object();
    rootPayload["contents"] =  nlohmann::json::array();
    for (const auto& msg : messageHistory) {

        // 3. Build the individual node containing the text block
        nlohmann::json messageNode = nlohmann::json::object();
        messageNode["role"] = msg.Role;
        messageNode["parts"] = nlohmann::json::array({
            nlohmann::json::object({ {"text", msg.Content} })
        });

        // 4. Append this message frame to the array
        rootPayload["contents"].push_back(messageNode);
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