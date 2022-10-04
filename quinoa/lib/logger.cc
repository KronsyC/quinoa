#include "./logger.h"
#include<ctime>
#include<iomanip>

#define DEBUG_MODE 0
using namespace std;

void Logger::writeLog(string message, Logger::LogLevel level)
{
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
    auto t = time(nullptr);
    auto tm = *localtime(&t);
    auto formattedTime = put_time(&tm, "%H:%M:%S");
    stringstream st;
    st << formattedTime;
    auto time = st.str();
    printf("%s [%s\e[0;0m]: %s\n", time.c_str(), prefix.c_str(), message.c_str());
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