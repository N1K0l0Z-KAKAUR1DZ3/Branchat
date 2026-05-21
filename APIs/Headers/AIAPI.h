#pragma once
#include "../../Dependencies/json.hpp"
#include "../../Models/Headers/Message.h"

#ifndef BRANCHAT1_AIAPI_H
#define BRANCHAT1_AIAPI_H
class AIAPI {
public:
    static void init();
    static Message GetAIResponse(std::vector<Message>& contextPayload);
};
#endif //BRANCHAT1_AIAPI_H
