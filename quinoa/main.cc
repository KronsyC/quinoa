

#include "./core/compiler.h"
#include "stdio.h"
#include <iostream>
#include <signal.h>
#include <string>
#include <vector>
#include "sys/stat.h"
#include "llvm/Support/Host.h"
#define TMP_DIR (std::string(QUINOA_DIR) + "/tmp")

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
//    error("SigAbrt", true);
}


void assert_create_dirs(std::initializer_list<std::string> paths){
    struct stat st = {0};

    for(auto path : paths){
        if (stat(path.c_str(), &st) == -1) {
            mkdir(TMP_DIR.c_str(), 0700);
        }
    }

}

int main(int argc, char** argv)
{

    // initialize random numbers
    srand((unsigned)time(NULL) * getpid());
    // Handle Segfaults Nicely
    signal(SIGSEGV, segfault);
    signal(SIGILL, ill);
    signal(SIGABRT, abort);

    assert_create_dirs({
        TMP_DIR
    });

    ClargParser parser;

    parser.add_clarg("o", "Output File", "quinoa_app");
    parser.add_clarg("target_triple",
                     "The compilation target triple to be used for the output, defaults to the host's triple",
                     llvm::sys::getDefaultTargetTriple().c_str()
     );

    parser.parse_clargs(argc, argv);


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