#include "../Headers/Message.h"

static std::string formatSqlTimestamp(const std::chrono::system_clock::time_point& timePoint) {
    const std::time_t unixTime = std::chrono::system_clock::to_time_t(timePoint);
    const std::tm* timeStruct = std::localtime(&unixTime);
    std::ostringstream oss;
    oss << std::put_time(timeStruct, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void Message::Print() const  {
    std::cout << "\t\t\t"<< formatSqlTimestamp(Timestamp) <<" - "<<  Role << ": "<< Content << std::endl;
}
