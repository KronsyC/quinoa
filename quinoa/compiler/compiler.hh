#pragma once
#include "llvm/IR/Module.h"
#include "./Lexer/lexer.hh"

void compile(std::string sourceCode){
    // Lex the file into a token vector
    auto tokens = Lexer::lexify(sourceCode);
}