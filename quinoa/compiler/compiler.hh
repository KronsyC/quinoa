#pragma once
#include "llvm/IR/Module.h"
#include "./Lexer/lexer.h"
#include "./Parser/parser.h"

void compile(std::string sourceCode){

    // Lex the file into a token vector
    auto tokens = Lexer::lexify(sourceCode);

    // Assemble an AST from the tokens
    auto ast = Parser::makeAst(tokens);




    // Print out the tokens
    // for(auto t:tokens){
    //     printf("-> %s\n", t.value.c_str());
    // }
}