#pragma once
#include<string>
#include "llvm/IR/Module.h"
#include "./AST/ast.hh"
CompilationUnit makeAst(std::string source);
std::string readFile(std::string path);
llvm::Module* createModule(std::string sourceCode, bool log = false);
std::string compile(std::string source);