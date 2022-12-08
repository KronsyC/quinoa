#pragma once

#include <utility>

#include "../token/token.h"
#include "../../lib/logger.h"
#include "../AST/symbol_operators.hh"
#include "../AST/advanced_operators.hh"
#include "../AST/control_flow.hh"
#include "../AST/intrinsic.hh"
//
// pop-with-error
// pops a token from the list, and errors if it is not a specific type
//
std::unique_ptr<LongName> parse_long_name(std::vector<Token> &toks);
std::vector<std::vector<Token>> parse_tst(std::vector<Token> &toks, TokenType at);

std::vector<std::vector<Token>> parse_cst(std::vector<Token> &toks);
Token pope(std::vector<Token> &toks, TokenType typ);
std::vector<Token> read_to(std::vector<Token> &toks, TokenType type);

std::vector<Token> read_to_reverse(std::vector<Token> &toks, TokenType type);


enum IndType {
    INDENTATION_TYPES
    IND_angles,
    IND_generics,
};
std::string get_tt_name(TokenType t);
void print_toks(const std::vector<Token> &toks);
void expects(Token tok, std::initializer_list<TokenType> expected_types);
void expects(Token tok, TokenType expected_type);
std::vector<Token> read_block(std::vector<Token> &toks, IndType typ);
