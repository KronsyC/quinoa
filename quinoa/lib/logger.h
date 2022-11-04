#pragma once
#include <string>

namespace Logger
{

enum LogLevel {
    LL_INFO,
    LL_DEBUG,
    LL_WARN,
    LL_ERR,
};
void writeLog(std::string message, LogLevel level);

void log(std::string message);
void debug(std::string message);
void warn(std::string message);
void error(std::string message);
void enqueueMode(bool mode);
void printQueue();
void clearQueue();
}