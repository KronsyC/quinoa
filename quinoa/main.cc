

#include "stdio.h"
#include <vector>
#include "./lib/error.h"
#include "./lib/logger.h"
#include "./compiler/AST/ast.hh"
#include "./compiler/compiler.h"
#include <string>
#include <iostream>
#include <fstream>
#include <signal.h>

using namespace std;

void segfault(int sig)
{
    error("Segfault", true);
}
void ill(int sig)
{
    error("SigIll", true);
}

void abort(int sig)
{
    // error("SigAbrt", true);
}

string buildIr(string path)
{
    Logger::log("Building file '" + path + "'");
    auto file = readFile(path);
    auto ir = compile(file, path);
    return ir;
}


int main(int argc, char **argv)
{
    // initialize random numbers
    srand((unsigned)time(NULL) * getpid());

    // Handle Segfaults Nicely
    signal(SIGSEGV, segfault);
    signal(SIGILL, ill);
    signal(SIGABRT, abort);
    if (argc < 2)except(E_BAD_ARGS, "The compiler expects at least ONE argument. Use the 'help' command for usage information");
    string command = argv[1];
    // Compile the file
    if (command == "build")
    {
        if (argc < 3)except(E_BAD_ARGS, "The 'build' command expects a target file path");
        string filePath = argv[2];

        auto ir = buildIr(filePath);

        ofstream out("test.ll");
        out.write(ir.c_str(), ir.size());
    }
    else except(E_BAD_ARGS, "Unrecognized Command '" + command + "'");
    return 0;
};