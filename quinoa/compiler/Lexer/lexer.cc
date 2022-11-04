#include "./lexer.h"
#include "../../lib/error.h"
#include "../../lib/list.h"
#include "../../lib/logger.h"
#include "../token/token.h"
#include <bits/stdc++.h>
#include <string>
#include <vector>
using namespace std;
int row = 1;
int col = 1;
static std::string filename;
void newline()
{
    row++;
    col = 1;
}
Token make(TokenType type, std::string value = "")
{
    Token tok(type, value, row, col, filename);
    return tok;
}
void trimStart(string& str)
{
    // Ascii Table Shennanigans
    while(str.length() && str[0] <= 32) {
	if(str[0] == '\n')
	    newline();
	else
	    col++;
	popf(str);
    }
}
bool startsWith(string str, std::string substr)
{
    if(substr.size() > str.size())
	return false;
    if(substr.size() == 0)
	return true;
    if(substr.size() == 1)
	return str[0] == substr[0];
    for(unsigned int i = 0; i < substr.size(); ++i) {
	auto tgt = str[i];
	auto cmp = substr[i];
	if(tgt != cmp)
	    return false;
    }
    return true;
}
bool isNumber(char c)
{
    return c >= '0' && c <= '9';
}
bool isAlphaChar(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
bool isExtVarChar(char c)
{
    return c == '_' || c == '$';
}
bool isSymbol(char c)
{
    return !isNumber(c) && !isAlphaChar(c);
}
bool isSymbol(string str)
{
    for(auto c : str) {
	if(!isSymbol(c))
	    return false;
    }
    return true;
}
string escapeNextVal(string& str)
{
    auto first = popf(str);
    switch(first) {
    case '"':
	return "\"";

    case 'n':
	return "\n";
    }
    except(E_UNESCAPABLE, "Failed To Escape string");
    return "";
}
bool compareLength(string s1, std::string s2)
{
    return s1.length() > s2.length();
}

inline vector<string> get_aliases()
{
    // Keyword Check (this is black magic, god have mercy on my soul)
    vector<string> aliases;
    // Construct a vector with all of the aliases, sorted by length (big->small)
    for(auto def : defs) {
	for(auto alias : def->alias) {
	    aliases.push_back(alias);
	}
    }
    sort(aliases.begin(), aliases.end(), compareLength);
    return aliases;
}

Token readNextToken(string& str)
{
    // Trim
    trimStart(str);
    if(str.length() == 0)
	return make(TT_eof);

    // Comment Check
    if(startsWith(str, "//")) {
	while(str[0] != '\n')
	    popf(str);
	popf(str);
	return readNextToken(str);
    }
    if(startsWith(str, "/*")) {
	while(!startsWith(str, "*/"))
	    popf(str);
	popf(str);
	popf(str);
	return readNextToken(str);
    }

    // string Check
    if(startsWith(str, "\"")) {
	std::string val;
	popf(str);
	bool escapeNext = false;
	while(str.length()) {
	    if(escapeNext) {
		val.append(escapeNextVal(str));
		escapeNext = false;
		continue;
	    } else if(str[0] == '\\') {
		popf(str);
		escapeNext = true;
		continue;
	    } else if(str[0] == '"') {
		popf(str);
		break;
	    } else {

		val += popf(str);
	    }
	}
	// TODO: This isn't actually accurate due to the width of escaped characters being smaller than their
	// source-code variants
	col += val.size() + 2;
	return make(TT_literal_str, val);
    }

    // Number Check
    if(isNumber(str[0])) {
	std::string constructedNumber;
	while(isNumber(str[0])) {
	    constructedNumber += str[0];
	    popf(str);
	}
	col += constructedNumber.size();
	return make(TT_literal_int, constructedNumber);
    }

    static auto aliases = get_aliases();

    for(auto a : aliases) {
	// Check for a match
	if(startsWith(str, a)) {
	    auto symbol = isSymbol(a);
	    if(!symbol) {
		// ensure the character after the keyword isn't an alphachar
		auto next = str[a.size()];
		if(isAlphaChar(next))
		    continue;
	    }
	    // Locate the appropriate def object and create an appropriate token type
	    for(auto d : defs) {
		if(includes(d->alias, a)) {
		    // skip n tokens ahead;
		    for(unsigned int i = 0; i < a.size(); i++) {
			popf(str);
		    }
		    col += a.size();
		    return make(d->ttype, a);
		}
	    }
	}
    }

    // Literal Parsing, accepts literally Anything
    if(isAlphaChar(str[0]) || isExtVarChar(str[0])) {
	std::string ident;
	while(str.length() && (isAlphaChar(str[0]) || isNumber(str[0]) || isExtVarChar(str[0]))) {
	    ident += popf(str);
	}
	col += ident.size();
	if(ident != "")
	    return make(TT_identifier, ident);
    }
    std::string s;
    s += str[0];
    except(E_UNREADABLE_CHAR, "Failed To Parse The File, An Unreadable Character '" + s + "' was Encountered");
    throw exception();
}

vector<Token> Lexer::lexify(string source, std::string fname)
{
    vector<Token> toks;
    filename = fname;
    while(1) {
	auto nextToken = readNextToken(source);
	if(nextToken.is(TT_eof))
	    break;
	toks.push_back(nextToken);
    }
    row = 1;
    col = 1;
    return toks;
}