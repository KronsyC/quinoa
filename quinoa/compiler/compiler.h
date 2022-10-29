#pragma once
#include<string>
#include "llvm/IR/Module.h"
#include "./AST/ast.hh"
CompilationUnit* makeAst(std::string source, std::string path, bool process=true);
std::string readFile(std::string path);
llvm::Module* createModule(std::string sourceCode, std::string path, bool log = false);
std::string compile(std::string source, std::string path);