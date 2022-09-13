#include "stdio.h"
#include<vector>
#include "./lib/error.h"
#include "./lib/logger.h"
#include<string>
using namespace std;
int main(int argc, char** argv){
    if(argc < 2)error("You Must Pass the File Path to the compiler");
    string filePath = argv[1];
    Logger::log("Compiling file '"+filePath+"'");
    auto execPath = argv[0];
    return 0;
};