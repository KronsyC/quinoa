#include "./core/compiler.h"
#include "core/llvm_utils.h"
#include "lib/error.h"
#include "lib/logger.h"
#include "stdio.h"
#include "sys/stat.h"
#include "llvm/Support/Host.h"
#include <iostream>
#include <signal.h>
#include <string>
#include <vector>
#define TMP_DIR (std::string(QUINOA_DIR) + "/tmp")

using namespace std;

void segfault(int sig) { error("Segfault", true); }
void ill(int sig) { error("SigIll", true); }

void abort(int sig) { error("SigAbrt", true); }

void assert_create_dirs(std::initializer_list<std::string> paths) {
    struct stat st = {0};

    for (auto path : paths) {
        if (stat(path.c_str(), &st) == -1) {
            mkdir(TMP_DIR.c_str(), 0700);
        }
    }
}
std::map<std::string, std::string> parse_imports(std::string data) {
    std::map<std::string, std::string> ret;
    std::string current_key;
    std::string current_val;
    bool building_key = true;

    for (auto c : data) {
        if (building_key) {
            if (c == '#') {
                building_key = false;
                continue;
            }
            current_key += c;
        } else {
            if (c == ';') {
                building_key = true;
                ret[current_key] = current_val;
                current_key = "";
                current_val = "";
                continue;
            } else
                current_val += c;
        }
    }

    if (building_key)
        except(E_BAD_ARGS, "Malformed include arg");
    if (!current_key.empty()) {
        ret[current_key] = current_val;
    }
    return ret;
}
int main(int argc, char** argv) {

    // initialize random numbers
    srand((unsigned)time(NULL) * getpid());
    // Handle Segfaults Nicely
    signal(SIGSEGV, segfault);
    signal(SIGILL, ill);
    signal(SIGABRT, abort);

    assert_create_dirs({TMP_DIR});

    ClargParser parser;

    parser.add_clarg("o", "Output File", "quinoa_app");
    parser.add_clarg("target_triple",
                     "The compilation target triple to be used for the output, defaults to the host's triple",
                     llvm::sys::getDefaultTargetTriple().c_str());
    parser.add_clarg("i", "Modules to include in the build", "<none>");
    parser.parse_clargs(argc, argv);

    if (argc < 2) {
        except(E_BAD_ARGS, "The compiler expects at least ONE argument. Use the 'help' command for usage information");
    }
    string command = argv[1];
    // Compile the file
    if (command == "build") {
        if (argc < 3) {
            except(E_BAD_ARGS, "The 'build' command expects a target file path");
        }
        string file_path = argv[2];
        Logger::debug("Compiling file: " + file_path);
        auto includes = parser.get_clarg<std::string>("i");
        std::map<std::string, std::string> imports;
        if (includes != "<none>") {
            imports = parse_imports(includes);
        }
        compile(file_path, parser, imports);

    } else
        except(E_BAD_ARGS, "Unrecognized Command '" + command + "'");
    return 0;
};
