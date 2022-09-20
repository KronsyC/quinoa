#include "./parser.h"
#include "../../lib/list.h"
#include "../token/token.h"
#include "./util.hh"
#include "../AST/ast.hh"
#include<vector>
using namespace std;

void printToks(std::vector<Token> toks){
    for(auto t : toks){
        printf("-> %s\n", t.value.c_str());
    }
}

Expression* parseExpression(vector<Token>& toks){
    if(toks.size() == 0)error("Cannot Generate an Expression from no tokens");
    if(toks.size() == 1){
        switch(toks[0].type){
            case TT_literal_int:return new Integer(stoll(toks[0].value));
            case TT_literal_true: return new Boolean(true);
            case TT_literal_false: return new Boolean(false);
            case TT_literal_str: return new String(toks[0].value);
            case TT_literal_float: return new Float(stold(toks[0].value));
            case TT_identifier: return new Identifier(toks[0].value);
            default: error("Failed To Generate an Appropriate Constant Value for '"+getTokenTypeName(toks[0].type)+"'");
        }
    }
    printToks(toks);
    return nullptr;
}

void* parseSourceBlock(vector<Token>& toks){
    while(toks.size()){
        auto line = readUntil(toks, TT_semicolon, true);
        auto f = line[0];


        if(f.is(TT_return)){
            popf(line);
            auto returnValue = parseExpression(line);
            continue;
        }

        // Default to expression parsing
        parseExpression(line);
    }
    return nullptr;
}

void* parseModuleContent(vector<Token>& toks){
    while(toks.size()){
        auto c = popf(toks);

        if(c.isTypeTok()){
            auto nameTok = popf(toks);
            expects(nameTok, TT_identifier);
            if(toks[0].is(TT_l_paren)){
                printf("function:: %s %s\n", c.value.c_str(), nameTok.value.c_str());
                auto argsTokens = readBlock(toks, IND_parens);
                auto contentToks = readBlock(toks, IND_braces);

                auto srcAst = parseSourceBlock(contentToks);
                continue;
            }
        }
        error("Functions are the only module members currently supported");

    }
    return nullptr;
}

void* Parser::makeAst(vector<Token>& toks){

    while(toks.size() ){
        auto c = popf(toks);

        // Top-Level Parsing
        switch(c.type){
            case TT_import:{
                auto importExprToks = readUntil(toks, TT_semicolon, true);
                if(importExprToks[0].is(TT_at_symbol)){
                    popf(importExprToks);
                    // Import from std lib
                }
                // Import Path foo.bar.baz
                break;
            }
            case TT_module:{
                auto name = popf(toks);
                if(toks[0].is(TT_is)){
                    error("Composition Syntax is not supported");
                }
                printf("module %s\n", name.value.c_str());
                auto moduleToks = readBlock(toks, IND_braces);
                auto content = parseModuleContent(moduleToks);
                printToks(moduleToks);
                break;
            }
            default:error("Failed to parse Token '"+c.value+"'");
        }
    }
    return nullptr;
}