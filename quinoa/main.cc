#include "stdio.h"
#include <vector>
#include "./lib/error.h"
#include "./lib/logger.h"
#include "./compiler/AST/ast.hh"
#include "./compiler/compiler.h"
#include <string>
#include <fstream>
using namespace std;
int main(int argc, char **argv)
{
    if (argc < 2)
        error("You Must Pass Options To The Compiler [INSERT INFO PAGE HERE]");
    string command = argv[1];
    if (command == "build")
    {
        if(argc <3 )error("Must Provide a File Path to the build command");
        string filePath = argv[2];
        Logger::log("Compiling file '" + filePath + "'");
        auto file = readFile(filePath);
        auto ir = compile(file);
        ofstream out("test.ll");
        out.write(ir.c_str(), ir.size());
    }
    else
        error("Unrecognized Command '" + command + "'");

    return 0;
};