#include "./logger.h"
#include<vector>
#define DEBUG_MODE 1
using namespace std;
static bool enqueue = false;
static std::vector<std::pair<Logger::LogLevel, std::string>> queue;

void Logger::printQueue(){
    auto initial = enqueue;
    enqueueMode(false);
    for(auto e:queue){
        writeLog(e.second, e.first);
    }
    enqueueMode(initial);
    
}
void Logger::enqueueMode(bool m){
    enqueue = m;
}
void Logger::writeLog(string message, Logger::LogLevel level)
{
    if(enqueue){
        queue.push_back({level, message});
        return;
    }
    string prefix;
    switch (level)
    {
    case Logger::LL_INFO:
        prefix = "\e[0;36mINFO";
        break;
    case Logger::LL_DEBUG:
        prefix = "\e[0;32mDEBUG";
        break;
    case Logger::LL_WARN:
        prefix = "\e[0;33mWARN";
        break;
    case Logger::LL_ERR:
        prefix = "\e[0;31mERROR";
        break;
    }
    printf("[%s\e[0;0m]: %s\n", prefix.c_str(), message.c_str());
}

void Logger::log(string message)
{
    Logger::writeLog(message, Logger::LL_INFO);
}
void Logger::debug(string message)
{
    if(DEBUG_MODE)Logger::writeLog(message, Logger::LL_DEBUG);
}
void Logger::warn(string message)
{
    Logger::writeLog(message, Logger::LL_WARN);
}


void Logger::error(string message)
{
    Logger::writeLog(message, Logger::LL_ERR);
}

void Logger::clearQueue(){
    queue.clear();
}
