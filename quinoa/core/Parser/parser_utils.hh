#pragma once

#include "../token/token.h"
#include "../../lib/logger.h"

/**
 *
 * Define Various Helper Functions, very useful
 * in the parsing process
 *
 */

std::string get_tt_name(TokenType t) {
    for (auto d: defs) {
        if (d->ttype == t) {
            return d->name;
        }
    }
    return "unknown";
}

void print_toks(std::vector <Token> toks) {
    std::string output;
    int max_name_len = 0;
    for (auto t: toks) {
        if (get_tt_name(t.type).size() > max_name_len)max_name_len = get_tt_name(t.type).size();
    }
    for (auto t: toks) {
        auto name = get_tt_name(t.type);
        output += "\t> ";
        output += name;
        for (int i = 0; i < max_name_len - name.size(); i++) {
            output += " ";
        }
        output += " -> " + t.value;
        output += "\n";
    }

    Logger::debug("TOKEN DUMP:\n" + output);
}

void expects(Token tok, std::initializer_list <TokenType> expected_types) {
    bool is_good = false;
    for (auto tt: expected_types) {
        if (tok.is(tt)) {
            is_good = true;
            break;
        }
    }

    if (!is_good) {
        std::string message = "Unexpected " + get_tt_name(tok.type) + " '" + tok.value + "'.";
        message += "\n\tWas Expecting: ";
        bool first = true;
        for (auto t: expected_types) {
            if (!first)
                message += " | ";

            message += "'" + get_tt_name(t) + "'";
            first = false;
        }
        except(E_UNEXPECTED_TOKEN, message);
    }
}

void expects(Token tok, TokenType expected_type) {
    expects(tok, {expected_type});
}

//
// pop-with-error
// pops a token from the list, and errors if it is not a specific type
//
inline Token pope(std::vector <Token> &toks, TokenType typ) {
    auto tok = popf(toks);
    expects(tok, typ);
    return tok;
}

std::vector <Token> read_to(std::vector <Token> &toks, TokenType type) {
    auto initial = toks;
    std::vector <Token> ret;
    int ind = 0;
    while (toks.size()) {
        if (toks[0].is(type) && ind == 0) {
            break;
        }
        if (toks[0].isIndentationTok())ind++;
        else if (toks[0].isDeIndentationTok())ind--;
        ret.push_back(popf(toks));

    }
    if (toks.size() == 0) {
        toks = initial;
        ret = {};
    }
    return ret;
}

std::vector <Token> read_to_reverse(std::vector <Token> &toks, TokenType type) {
    auto initial = toks;
    std::vector <Token> ret;
    int ind = 0;
    while (toks.size()) {
        auto end = toks[toks.size() - 1];

        if (end.is(type) && ind == 0) {
            break;
        }
        pushf(ret, end);

        if (end.isDeIndentationTok())ind++;
        else if (end.isIndentationTok())ind--;
        toks.pop_back();
    }
    if (toks.size() == 0) {
        toks = initial;
        ret = {};
    }
    return ret;
}


enum IndType {
    INDENTATION_TYPES
};

std::map <IndType, std::pair<TokenType, TokenType>> mappings{INDENTATION_MAPPINGS};

std::vector <Token> read_block(std::vector <Token> &toks, IndType typ) {
    auto p = mappings[typ];
    auto i = p.first;
    auto u = p.second;
    auto first_tok = pope(toks, i);

    int ind = 1;
    std::vector <Token> ret;
    while (toks.size()) {
        auto t = popf(toks);
        if (t.is(i))
            ind++;
        if (t.is(u))
            ind--;
        if (ind == 0)
            break;
        ret.push_back(t);
    }
    if (ind != 0)
        error("Expected corresponding '" + get_tt_name(u) + "' to " + get_tt_name(first_tok.type) + " at " +
              first_tok.afterpos());
    return ret;
}
