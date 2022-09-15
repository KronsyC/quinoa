#include "stdio.h"
#include<vector>
#include "./lib/error.h"
#include "./lib/logger.h"
#include "./compiler/AST/ast.hh"
#include "./compiler/compiler.hh"
#include<string>
class Thing{
public:
    Thing() = default;
};
using namespace std;
int main(int argc, char** argv){
    if(argc < 2)error("You Must Pass the File Path to the compiler");
    string filePath = argv[1];
    Logger::log("Compiling file '"+filePath+"'");
    Block<Thing> stuff;
    Thing th;
    stuff.push(th);
    auto execPath = argv[0];
    for(auto t:defs){
        printf("%s\n", t->name.c_str());
    }
    return 0;
};