#include "./parser_utils.hh"

#include <utility>

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

std::vector<std::vector<Token>> parse_tst(std::vector<Token> &toks, TokenType at)
{
    if (toks.size() == 0)
        return {};

    static std::vector<TokenType> indt, dindt;
    if(!indt.size()){
        for (auto d : defs)
        {
            if (d->ind)
                indt.push_back(d->ttype);
            else if (d->dind)
                dindt.push_back(d->ttype);
        }
    }

    std::vector<std::vector<Token>> retVal;
    std::vector<Token> temp;
    int ind = 0;
    while (toks.size())
    {
        auto t = popf(toks);
        if (includes(indt, t.type))
            ind++;
        if (includes(dindt, t.type))
            ind--;

        if (ind == 0 && t.is(at))
        {
            retVal.push_back(temp);
            temp.clear();
        }
        else
        {
            temp.push_back(t);
        }
    }
    retVal.push_back(temp);
    return retVal;
}


std::vector<std::vector<Token>> parse_cst(std::vector<Token> &toks){
  return parse_tst(toks, TT_comma);
}

std::unique_ptr<LongName> parse_long_name(std::vector<Token> &toks)
{
    auto name = std::make_unique<LongName>();

    bool expects_split = false;
    while (!toks.empty())
    {
        auto current = toks[0];
        if (expects_split)
        {
            if (current.is(TT_double_colon))
            {
                expects_split = false;
                popf(toks);
                continue;
            }
            else
                break;
        }
        else
        {
            popf(toks);
            // 100% has to be an identifier
            expects(current, TT_identifier);
            Name segment(current.value);
            name->parts.push(segment);
            expects_split = true;
        }
    }
    return name;
}
void print_toks(const std::vector<Token> &toks) {
    std::string output;
    unsigned int max_name_len = 0;
    for (const auto &t: toks) {
        if (get_tt_name(t.type).size() > max_name_len) {
            max_name_len = get_tt_name(t.type).size();
        }
    }
    for (const auto &t: toks) {
        auto name = get_tt_name(t.type);
        output += "\t> ";
        output += name;
        for (unsigned int i = 0; i < max_name_len - name.size(); i++) {
            output += " ";
        }
        output += " -> " + t.value;
        output += "\n";
    }

    Logger::debug("TOKEN DUMP:\n" + output);
}

void expects(Token tok, std::initializer_list<TokenType> expected_types) {
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
    expects(std::move(tok), {expected_type});
}

//
// pop-with-error
// pops a token from the list, and errors if it is not a specific type
//
Token pope(std::vector<Token> &toks, TokenType typ) {
    auto tok = popf(toks);
    expects(tok, typ);
    return tok;
}

std::vector<Token> read_to(std::vector<Token> &toks, TokenType type) {
    auto initial = toks;
    std::vector<Token> ret;
    int ind = 0;
    while (!toks.empty()) {
        if (toks[0].is(type) && ind == 0) {
            break;
        }
        if (toks[0].isIndentationTok())ind++;
        else if (toks[0].isDeIndentationTok())ind--;
        ret.push_back(popf(toks));

    }
    if (toks.empty()) {
        toks = initial;
        ret = {};
    }
    return ret;
}

std::vector<Token> read_to_reverse(std::vector<Token> &toks, TokenType type) {
    auto initial = toks;
    std::vector<Token> ret;
    int ind = 0;
    while (!toks.empty()) {
        auto end = toks[toks.size() - 1];

        if (end.is(type) && ind == 0) {
            break;
        }
        pushf(ret, end);

        if (end.isDeIndentationTok())ind++;
        else if (end.isIndentationTok())ind--;
        toks.pop_back();
    }
    if (toks.empty()) {
        toks = initial;
        ret = {};
    }
    return ret;
}


std::map<IndType, std::pair<TokenType, TokenType>> mappings{INDENTATION_MAPPINGS {IND_angles, {TT_lesser, TT_greater}}, {IND_generics, {TT_op_generic, TT_greater}} };

#include<variant>

std::variant<std::vector<Token>, std::string> parse_block(std::vector<Token> &toks, IndType typ){
    auto p = mappings[typ];
    auto i = p.first;
    auto u = p.second;
    auto first_tok = pope(toks, i);

    int ind = 1;
    std::vector<Token> ret;
    while (!toks.empty()) {
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
        return "Expected corresponding '" + get_tt_name(u) + "' to " + get_tt_name(first_tok.type) + " at " +
              first_tok.afterpos();
    return ret;
}

std::vector<Token> read_block(std::vector<Token> &toks, IndType typ) {
  auto result = parse_block(toks, typ);
  
  if(auto err = std::get_if<std::string>(&result))except(E_INTERNAL, *err);
  return *std::get_if<std::vector<Token>>(&result);
}
