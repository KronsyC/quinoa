#pragma once
#include "./AST/compilation_unit.hh"
#include "llvm/IR/Module.h"
#include <string>
#include "../lib/clarg_parser.hh"
std::unique_ptr<CompilationUnit> make_ast(std::string source, std::string path, bool process = true);
std::string read_file(std::string path);
llvm::Module* createModule(std::string sourceCode, std::string path, bool log = false);
void compile(std::string path, ClargParser& clargs);