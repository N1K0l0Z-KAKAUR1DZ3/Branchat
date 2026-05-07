#include <memory>
#include <string>
#include <vector>
#include <chrono>

struct Message {
    int ChatID;
    std::string id;
    std::string role;
    std::string content;
    std::string model;
    int prompt_tokens = 0;
    int completion_tokens = 0;
    int64_t timestamp;
    std::string finish_reason;
    std::string refusal;

    Message(std::string r, std::string c, std::string m = "unknown", const int ChatId = -1) : ChatID(ChatId), role(std::move(r)), content(std::move(c)), model(std::move(m)) {
        const auto now = std::chrono::system_clock::now();
        timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    }
};

struct RootChat {
    std::string name;
    std::vector<Message> messages;
    int id;
    explicit RootChat(std::string _name, std::vector<Message> _messages = {} , const int _id = -1) : name(std::move(_name)), messages(std::move(_messages)), id(_id){}
    void CreateSubChat(){}
};

struct Chat : RootChat{
    int parentId;
    std::string context;
    std::vector<Message> messages;
    int rootId;

    explicit Chat(const int _parentId, std::string _context, std::string _name, std::vector<Message>& _messages, const int _rootId, const int _id = -1) : RootChat(std::move(_name), std::move(_messages), _id),
    parentId(_parentId),
    context(std::move(_context)),
    messages(std::move(_messages)),
    rootId(_rootId){}
};




