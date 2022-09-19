#include "./parser.h"
#include "../../lib/list.h"
#include "../token/token.h"
#include "./util.hh"
#include<vector>
using namespace std;

void printToks(std::vector<Token> toks){
    for(auto t : toks){
        printf("-> %s\n", t.value.c_str());
    }
}


void* Parser::makeAst(vector<Token>& toks){

    while(toks.size() > 0){
        auto c = popf(toks);

        // Top-Level Parsing
        switch(c.type){
            case TT_import:{
                auto importExprToks = readUntil(toks, TT_semicolon, true);
                break;
            }
            case TT_module:{
                auto name = popf(toks);
                printf("module %s\n", name.value.c_str());
                auto moduleToks = readBlock(toks, IND_braces);
                break;
            }
            default:error("Failed to parse Token '"+c.value+"'");
        }
    }
    return nullptr;
}