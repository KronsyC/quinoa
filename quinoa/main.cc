

#include "./compiler/AST/ast.hh"
#include "./compiler/compiler.h"
#include "./lib/error.h"
#include "./lib/logger.h"
#include "./lib/clarg_parser.hh"
#include "stdio.h"
#include <fstream>
#include <iostream>
#include <signal.h>
#include <string>
#include <vector>

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
    error("SigAbrt", true);
}


int main(int argc, char** argv)
{
    // initialize random numbers
    srand((unsigned)time(NULL) * getpid());

    // Handle Segfaults Nicely
    signal(SIGSEGV, segfault);
    signal(SIGILL, ill);
    signal(SIGABRT, abort);

    ClargParser parser;
    parser.add_clarg<std::string>("o", "Output File", "q_app");
    
    parser.parse_clargs(argc, argv);

    auto output_path = parser.get_clarg<std::string>("o");
    Logger::debug("output: " + output_path);

    if(argc < 2){
	    except(E_BAD_ARGS, "The compiler expects at least ONE argument. Use the 'help' command for usage information");
    }
    string command = argv[1];
    // Compile the file
    if(command == "build") {
        if(argc < 3){
            except(E_BAD_ARGS, "The 'build' command expects a target file path");

        }
	string file_path = argv[2];

	compile(file_path, parser);

    } else
	except(E_BAD_ARGS, "Unrecognized Command '" + command + "'");
    return 0;
};