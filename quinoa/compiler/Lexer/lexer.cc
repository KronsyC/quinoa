#include "./lexer.hh"
#include<vector>
#include<string>
#include "./Token.h"
#include "../../lib/list.h"
#include "../../lib/logger.h"
using namespace std;

Token make(TokenType type, string value=""){
    Token tok;
    tok.type = type;
    tok.value = value;
    return tok;
}
void trimStart(string& str){
    // Ascii Table Shennanigans
    while(str[0] <= 32)popf(str);
}
bool startsWith(string str, string substr){
    if(substr.size() > str.size())return false;
    if(substr.size() == 0)return true;
    if(substr.size() == 1)return str[0] == substr[0];
    for(int i = 0; i<str.size(); ++i){
        auto tgt = str[i];
        auto cmp = substr[i];
        if(tgt != cmp)return false;
    }
    return true;
}
string escapeNextVal(string& str){
    auto first =  popf(str);
    switch(first){
        case '"':{
            return "\"";
        }
    }
    Logger::error("Failed To Escape String");
}
Token readNextToken(string& str){
    // Trim
    trimStart(str);
    if(str.length()==0)return make(TT_eof);
    printf("processing\n%s\n", str.c_str());

    // String Check
    if(startsWith(str, "\"")){
        string val;
        popf(str);
        bool escapeNext = false;
        while(str.length()){
            if(escapeNext){
                val.append(escapeNextVal(str));
                escapeNext=false;
                continue;
            }
            else if(str[0]=='\\'){
                popf(str);
                escapeNext = true;
                continue;
            }
            else if(str[0]=='"'){
                popf(str);
                break;
            }
            else{
                
                val+=popf(str);
            }
        }
        printf("Parsed String -> %s\n", val.c_str());
        return make(TT_literal_str, val);
    }
    
}

vector<Token> Lexer::lexify(string source){
    while(source.length() > 0){
        auto nextToken = readNextToken(source);
        
    }

    return {};
}  