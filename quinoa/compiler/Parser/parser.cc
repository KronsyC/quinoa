#include "./parser.h"
#include "../../lib/list.h"
#include "../token/token.h"
#include "./util.hh"
#include "../../lib/logger.h"
#include "../AST/ast.hh"
#include <vector>
using namespace std;

void printToks(std::vector<Token> toks, bool oneLine = false)
{
    if (oneLine)
    {
        string output;
        for(auto t:toks){
            output+=t.value;
        }
        printf("%s;\n", output.c_str());
    }
    else
    {
        for (auto t : toks)
        {
            printf("-> [%s] %s\n", getTokenTypeName(t.type).c_str(), t.value.c_str());
        }
    }
}
Identifier *parseIdentifier(vector<Token> &toks)
{
    expects(toks[0], TT_identifier);
    // Simplest Case (single-part)
    if (toks.size() < 3 || !(toks[1].is(TT_dot) && toks[2].is(TT_identifier)))
        return new Ident(popf(toks).value);
    vector<Identifier *> parts;
    bool expectDot = false;
    for (auto t : toks)
    {
        if (expectDot)
        {
            if (!t.is(TT_dot))
                break;
            expectDot = false;
            continue;
        }
        if (t.is(TT_identifier))
        {
            parts.push_back(new Ident(t.value));
            expectDot = true;
            continue;
        }
        break;
    }
    for (int i = 0; i < parts.size() * 2 - 1; i++)
        popf(toks);
    return new CompoundIdentifier(parts);
}

vector<vector<Token>> parseCommaSeparatedValues(vector<Token> &toks)
{
    vector<TokenType> indt;
    vector<TokenType> dindt;
    for (auto d : defs)
    {
        if (d->ind)
            indt.push_back(d->ttype);
        else if (d->dind)
            dindt.push_back(d->ttype);
    }
    vector<vector<Token>> retVal;
    vector<Token> temp;
    int ind = 0;
    while (toks.size())
    {
        auto t = popf(toks);
        if (includes(indt, t.type))
            ind++;
        if (includes(dindt, t.type))
            ind--;

        if (ind == 0 && t.is(TT_comma))
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
Expression *parseExpression(vector<Token> &toks)
{
    if (toks.size() == 0)
        error("Cannot Generate an Expression from no tokens");
    if (toks.size() == 1)
    {
        switch (toks[0].type)
        {
        case TT_literal_int:
            return new Integer(stoll(toks[0].value));
        case TT_literal_true:
            return new Boolean(true);
        case TT_literal_false:
            return new Boolean(false);
        case TT_literal_str:
            return new String(toks[0].value);
        case TT_literal_float:
            return new Float(stold(toks[0].value));
        case TT_identifier:
            return new Ident(toks[0].value);
        default:
            error("Failed To Generate an Appropriate Constant Value for '" + getTokenTypeName(toks[0].type) + "'");
        }
    }

    auto c = toks[0];

    if (c.is(TT_identifier))
    {
        auto initial = toks;
        auto target = parseIdentifier(toks);
        if (toks[0].is(TT_l_paren))
        {
            auto params = readBlock(toks, IND_parens);
            if (toks.size() == 0)
            {
                auto paramsList = parseCommaSeparatedValues(params);
                vector<Expression *> params;
                for (auto p : paramsList)
                {
                    auto expr = parseExpression(p);
                    params.push_back(expr);
                }

                MethodCall call;
                call.target = target;
                call.params = params;
                return new MethodCall(call);
            }
        }
        toks = initial;
    }

    if (c.is(TT_l_paren))
    {
        auto initial = toks;
        auto block = readBlock(toks, IND_parens);
        if (toks.size() == 0)
            return parseExpression(block);

        toks = initial;
    }
    // Build a Table of Operator Precedences
    std::map<TokenType, int> precedences;
    for (auto d : defs)
    {
        if (d->infix)
        {
            precedences[d->ttype] = d->infix;
        }
    }

    vector<vector<int>> weightMap;
    int depth = 0;
    for (int i = 0; i < toks.size(); i++)
    {
        auto t = toks[i];

        // parenthesis are unwrapped at the end, it's counter-intuitive to BIMDAS
        // but thats how it works in trees
        if (t.isIndentationTok())
            depth++;
        if (t.isDeIndentationTok())
            depth--;

        if (depth > 0)
            continue;

        int weight = precedences[t.type];
        if (weight == 0)
            continue;
        vector<int> entry = {i, weight};
        weightMap.push_back(entry);
    }

    // find the best place to split the expression
    int splitPoint = -1;
    int splitWeight = 0;
    for (int i = 0; i < weightMap.size(); i++)
    {
        auto pair = weightMap[i];
        int index = pair[0];
        int weight = pair[1];
        if (splitPoint == -1 || (splitWeight <= weight))
        {
            splitPoint = index;
            splitWeight = weight;
        }
    }
    if (splitPoint == -1)
    {
        printToks(toks);
        error("Failed To Parse Expression");
    }
    auto op = toks[splitPoint];
    auto [left, right] = split(toks, splitPoint);
    auto l = left;
    auto r = right;
    auto leftAST = parseExpression(left);
    auto rightAST = parseExpression(right);
    auto optype = binary_op_mappings[op.type];
    return new BinaryOperation(leftAST, rightAST, optype);
}

Block<Statement> *parseSourceBlock(vector<Token> &toks)
{
    Block<Statement> block;
    while (toks.size())
    {
        auto line = readUntil(toks, TT_semicolon, true);
        auto f = line[0];

        if (f.is(TT_return))
        {
            popf(line);
            auto returnValue = parseExpression(line);
            block.push(new Return(returnValue));
            continue;
        }

        // Default to expression parsing
        auto expr = parseExpression(line);
        block.push(expr);
    }
    return new Block<Statement>(block);
}

void parseModuleContent(vector<Token> &toks, Module* mod)
{
    while (toks.size())
    {
        auto c = popf(toks);

        if (c.isTypeTok())
        {
            auto nameTok = popf(toks);
            expects(nameTok, TT_identifier);
            if (toks[0].is(TT_l_paren))
            {
                auto argsTokens = readBlock(toks, IND_parens);
                auto contentToks = readBlock(toks, IND_braces);

                auto srcAst = parseSourceBlock(contentToks);
                continue;
            }
        }
        error("Functions are the only module members currently supported");
    }
}

CompilationUnit* Parser::makeAst(vector<Token> &toks)
{
    auto unit = new CompilationUnit();
    while (toks.size())
    {
        auto c = popf(toks);

        // Top-Level Parsing
        switch (c.type)
        {
        case TT_import:
        {
            // Imports are handled/resolved in the preprocessor
            auto importExprToks = readUntil(toks, TT_semicolon, true);
            bool isStd = false;
            if (importExprToks[0].is(TT_at_symbol))
            {
                popf(importExprToks);
                isStd = true;
            }
            auto target = parseIdentifier(importExprToks);
            auto alias = target;
            if(importExprToks.size()){
                auto as = popf(importExprToks);
                expects(as, TT_as);
                if(importExprToks.size() == 0)error("Expected An Import Alias at " + as.afterpos());
                expects(importExprToks[0], TT_identifier);
                alias = new Ident(popf(importExprToks).value);
                if(importExprToks.size())error("An Import Alias may only be a single identifier");
            }
            unit->push(new Import(target, isStd, alias));
            // Import Path foo.bar.baz
            break;
        }
        case TT_module:
        {
            auto name = popf(toks);
            if (toks[0].is(TT_is))
            {
                error("Composition Syntax is yet to be supported (WIP)");
            }
            printf("module %s\n", name.value.c_str());
            auto moduleToks = readBlock(toks, IND_braces);
            auto mod = new Module();
            parseModuleContent(moduleToks, mod);

            mod->name = new Ident(name.value);
            unit->push(mod);
            break;
        }
        default:
            error("Failed to parse Token '" + c.value + "'");
        }
    }
    return unit;
}