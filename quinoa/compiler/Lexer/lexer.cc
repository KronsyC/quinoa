#include "./lexer.h"
#include<vector>
#include<string>
#include<bits/stdc++.h>
#include "../token/token.h"
#include "../../lib/logger.h"
#include "../../lib/list.h"
using namespace std;

Token make(TokenType type, string value=""){
    Token tok(type, value);
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
    for(int i = 0; i<substr.size(); ++i){
        auto tgt = str[i];
        auto cmp = substr[i];
        if(tgt != cmp)return false;
    }
    return true;
}
bool isNumber(char c){
    return c >= '0' && c <= '9';
}
bool isAlphaChar(char c){
    return c >= 'A' && c<='Z' || c>='a' && c<='z';
}
bool isSymbol(char c){
    return !isNumber(c) && !isAlphaChar(c);
}
bool isSymbol(string str){
    for(auto c:str){
        if(!isSymbol(c))return false;
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
    throw exception();
}
bool compareLength(string s1, string s2){
    return s1.length() > s2.length();
}
Token readNextToken(string& str){
    // Trim
    trimStart(str);
    if(str.length()==0)return make(TT_eof);

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
        return make(TT_literal_str, val);
    }
    
    // Number Check
    if(isNumber(str[0])){
        string constructedNumber;
        while(isNumber(str[0])){
            constructedNumber+=str[0];
            popf(str);
        }
        return make(TT_literal_int, constructedNumber);
    }

    // Keyword Check (this is black magic, god have mercy on my soul)
    vector<string> aliases;
    // Construct a vector with all of the aliases, sorted by length (big->small)
    for(auto def:defs){
        for(auto alias:def->alias){
            aliases.push_back(alias);
        }
    }
    sort(aliases.begin(), aliases.end(), compareLength);
    for(auto a:aliases){
        // Check for a match
        // printf("Checking %s\n", a.c_str());
        if(startsWith(str, a)){
            auto symbol = isSymbol(a);
            if(!symbol){
                // ensure the character after the keyword isn't an alphachar
                auto next = str[a.size()];
                if(isAlphaChar(next))continue;
            }
            // Locate the appropriate def object and create an appropriate token type
            for(auto d:defs){
                if(includes(d->alias, a)){
                    // skip n tokens ahead;
                    for(int i = 0; i<a.size();i++){
                        popf(str);
                    }
                    return make(d->ttype, d->name);
                }
            }

        }
    }
    
    // Literal Parsing, accepts literally Anything
    if(isAlphaChar(str[0])){
        string ident;
        while(str.length()&& (isAlphaChar(str[0]) || isNumber(str[0]))){
            ident+=popf(str);
        }
        return make(TT_identifier, ident);
    }
    string s;
    s+=str[0];
    Logger::error("Failed To Parse The File, An Unreadable Character '" + s + "' was Encountered");
    exit(1);
}   

vector<Token> Lexer::lexify(string source){
    vector<Token> toks;
    while(source.length() > 0){
        auto nextToken = readNextToken(source);
        if(nextToken.is(TT_eof))break;
        toks.push_back(nextToken);    
    }

    return toks;
}  