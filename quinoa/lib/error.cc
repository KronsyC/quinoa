#include "./error.h"
#include "./logger.h"
using namespace std;

void error(string reason){
    Logger::error(reason);
    exit(1);
}