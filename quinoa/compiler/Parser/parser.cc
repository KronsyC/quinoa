#include "./parser.h"
#include "../../lib/list.h"
#include "../token/token.h"
#include<vector>
using namespace std;

// Token popf(vector<Token>& toks);

vector<Token> readUntil(vector<Token>& toks, TokenType type, bool removeEnding=false){
    vector<Token> retval;
    while(toks.size()){
        if(toks[0].is(type))break;
        retval.push_back(popf(toks));
    }
    if(removeEnding)popf(toks);
    return retval;
}
void printToks(vector<Token> toks){
    for(auto t : toks){
        printf("-> %s\n", t.value.c_str());
    }
}
void* Parser::makeAst(vector<Token>& toks){

    while(toks.size() > 0){
        auto c = popf(toks);


        switch(c.type){
            case TT_import:{
                printf("Import\n");
                auto importExprToks = readUntil(toks, TT_semicolon, true);
                printToks(toks);
            }
        }
    }
    return nullptr;
}