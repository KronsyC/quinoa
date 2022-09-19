#pragma once
#include "llvm/IR/Module.h"
#include "./Lexer/lexer.h"
#include "./Parser/parser.h"
#include<vector>

void compile(std::string sourceCode){

    // Lex the file into a token vector
    auto tokens = Lexer::lexify(sourceCode);
    // Assemble an AST from the tokens
    auto ast = Parser::makeAst(tokens);





}