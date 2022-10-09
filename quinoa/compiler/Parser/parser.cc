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
    }
    else
    {
        for (auto t : toks)
        {
            printf("-> [%s] %s\n", getTokenTypeName(t.type).c_str(), t.value.c_str());
        }
    }
}
CompoundIdentifier *parseIdentifier(vector<Token> &toks)
{
    expects(toks[0], TT_identifier);
    // Simplest Case (single-part)
    if (toks.size() < 3 || !(toks[1].is(TT_double_colon) && toks[2].is(TT_identifier)))
        return new CompoundIdentifier(popf(toks).value);
    vector<Identifier *> parts;
    bool expectDot = false;
    for (auto t : toks)
    {
        if (expectDot)
        {
            if (!t.is(TT_double_colon))
                break;
            expectDot = false;
            continue;
        }
        if (t.is(TT_identifier))
        {
            parts.push_back(Ident::get(t.value));
            expectDot = true;
            continue;
        }
        break;
    }
    for (int i = 0; i < parts.size() * 2 - 1; i++)
        popf(toks);
    return new CompoundIdentifier(parts);
}
Type* parseType(vector<Token>& toks){
    if(!toks.size())error("Failed To Parse Type");
    Type* ret = nullptr;
    if(toks[0].isTypeTok())ret = Primitive::get(primitive_mappings[popf(toks).type]);
    else{
        auto references = parseIdentifier(toks);
        ret = new CustomType(references);
    }

    while(toks.size() && toks[0].is(TT_star)){
        popf(toks);
        ret = TPtr::get(ret);
    }
    if(toks.size() && toks[0].is(TT_l_square_bracket)){
        // Type Array
        auto size = readBlock(toks, IND_square_brackets);
        if(size.size()==0)ret=ListType::get(ret, nullptr);
        else{
            error("Variable Length Arrays are currently not supported");
        }
    }
    return ret;
}
vector<vector<Token>> parseCommaSeparatedValues(vector<Token> &toks)
{
    if(toks.size() == 0)return {};
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


Expression *parseExpression(vector<Token> &toks, LocalTypeTable type_info, SourceBlock* ctx)
{
    if (toks.size() == 0)
        error("Cannot Generate an Expression from no tokens");
    if (toks.size() == 1)
    {
        switch (toks[0].type)
        {
        case TT_literal_int:
            return new Integer(stoull(toks[0].value));
        case TT_literal_true:
            return new Boolean(true);
        case TT_literal_false:
            return new Boolean(false);
        case TT_literal_str: return new String(toks[0].value);
        case TT_literal_float:
            return new Float(stold(toks[0].value));
        case TT_identifier:
            return Ident::get(toks[0].value);
        default:
            error("Failed To Generate an Appropriate Constant Value for '" + getTokenTypeName(toks[0].type) + "'");
        }
    }

    auto c = toks[0];

    if (c.is(TT_identifier))
    {
        auto initial = toks;
        auto target = parseIdentifier(toks);
        target->ctx = ctx;
        if (toks[0].is(TT_l_paren))
        {
            auto params = readBlock(toks, IND_parens);
            if (toks.size() == 0)
            {
                int i = 1; 
                auto paramsList = parseCommaSeparatedValues(params);
                vector<Expression *> params;
                vector<Type*> types;
                for (auto p : paramsList)
                {
                    auto expr = parseExpression(p, type_info, ctx);
                    expr->ctx = ctx;
                    auto t = expr->getType(type_info);
                    types.push_back(t);
                    params.push_back(expr);
                }
                auto call = new MethodCall;
                call->params = params;
                call->target = nullptr;
                if(instanceof<CompoundIdentifier>(target))call->name = (CompoundIdentifier*)target;
                else{
                    auto id = new CompoundIdentifier(target->str());
                    id->ctx = ctx;
                    call->name = id;
                }
                return call;
            }
        }
        else if(toks[0].is(TT_l_square_bracket)){
            auto etoks = readBlock(toks, IND_square_brackets);
            if(toks.size() == 0){
                auto item = parseExpression(etoks, type_info, ctx);
                item->ctx = ctx;
                return new Subscript(target, item);
            }
        }
        toks = initial;
    }

    if (c.is(TT_l_paren))
    {
        auto initial = toks;
        auto block = readBlock(toks, IND_parens);
        if (toks.size() == 0)
            return parseExpression(block, type_info, ctx);

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
    auto leftAST = parseExpression(left, type_info, ctx);
    auto rightAST = parseExpression(right, type_info, ctx);
    auto optype = binary_op_mappings[op.type];
    return new BinaryOperation(leftAST, rightAST, optype);
}

SourceBlock* parseSourceBlock(vector<Token> toks, LocalTypeTable typeinfo={})
{
    auto block = new SourceBlock;
    auto type_info = new LocalTypeTable;
    *type_info = typeinfo;

    while (toks.size())
    {
        auto first = toks[0];
        if(first.is(TT_while)){
            popf(toks);
            auto expr = readBlock(toks, IND_parens);
            auto exec = readBlock(toks, IND_braces);
            auto cond = parseExpression(expr, *type_info, block);
            cond->ctx = block->self;
            auto content= parseSourceBlock(exec, *type_info);
            auto loop = new WhileCond;
            *loop = *(WhileCond*)content;
            loop->cond = cond;
            loop->ctx = block->self;
            // For some reason the loop becomes inactive at this point
            loop->active = true;
            block->push(loop);
            continue;
        }

        auto line = readUntil(toks, TT_semicolon, true);
        auto f = line[0];

        if (f.is(TT_return))
        {
            popf(line);
            auto returnValue = parseExpression(line, *type_info, block);
            auto ret = new Return(returnValue);
            ret->ctx = block->self;
            returnValue->ctx = block->self;
            block->push(ret);
            continue;
        }
        else if(f.is(TT_let)){
            popf(line);    
            auto varname = popf(line).value; 
            Type* vartype;
            if(line[0].is(TT_colon)){
                popf(line);
                vartype = parseType(line);
            }       
            else vartype = nullptr;
            auto name = Ident::get(varname);
            name->ctx = block->self;

            // Add the variable to the type table
            (*type_info)[varname] = vartype;
            block->declarations.push_back(varname);
            auto init = new InitializeVar(vartype, name);
            init->ctx = block->self;
            block->push(init);
            if(line.size() != 0){
                expects(popf(line), TT_assignment);
                auto val = parseExpression(line, *type_info, block);
                auto ass = new BinaryOperation(name, val, BIN_assignment);
                ass->ctx = block->self;
                val->ctx = block->self;
                block->push(ass);
            }
            continue;
        }

        // Default to expression parsing
        auto expr = parseExpression(line, *type_info, block);
        expr->ctx = block->self;

        block->push(expr);
    }

 

    block->local_types = type_info;

    return block;
}

Param* parseParameter(vector<Token>& toks){
    bool isVarParam = false;
    if(toks[0].is(TT_ellipsis)){
        isVarParam = true;
        popf(toks);
    }
    auto name = popf(toks);
    expects(name, TT_identifier);
    expects(popf(toks), TT_colon);
    auto type = parseType(toks);

    if(toks.size()){
        printToks(toks);
        error("Failed to Parse Parameter");
    }
    auto p = new Param(type, Ident::get(name.value));
    p->isVariadic = isVarParam;
    return p;
}

void parseModuleContent(vector<Token> &toks, Module* mod)
{
    while (toks.size())
    {
        auto c = popf(toks);

        if(c.is(TT_func)){
            auto nameTok = popf(toks);
            expects(nameTok, TT_identifier);
            expects(toks[0], TT_l_paren);
            auto argsTokens = readBlock(toks, IND_parens);

            Type* returnType;

            // Functions implicitly return void
            if(toks[0].is(TT_arrow)){
                popf(toks);
                returnType = parseType(toks);
            }
            else{
                returnType = Primitive::get(PR_void);
            }

            auto argsCSV = parseCommaSeparatedValues(argsTokens);
            vector<Param*> params;
            LocalTypeTable argTypes;
            for(auto a:argsCSV){
                auto param = parseParameter(a);
                argTypes[param->name->str()] = param->type;
                Logger::debug(param->name->str() + " : " + param->type->str());
                params.push_back(param);
            }
            auto method = new Method();
            auto sig = new MethodSignature();
            if(toks[0].is(TT_l_brace)){
               auto contentToks = readBlock(toks, IND_braces);
                auto content = parseSourceBlock(contentToks, argTypes);
                *method = *static_cast<Method*>(content); 
            }
            else{
                 expects(toks[0], TT_semicolon);
                 popf(toks);
                 method->local_types = new LocalTypeTable;
            }

            // delete content;
            method->sig = sig;

            sig->name = Ident::get(nameTok.value);
            sig->space = mod->name;
            sig->params = params;
            sig->returnType = returnType;
            mod->push(method);

            continue;
            
        }

        error("Functions are the only module members currently supported");
    }
}

ModuleReference* parseCompositor(vector<Token>& toks){
    auto name = parseIdentifier(toks);
    if(toks.size())error("Complex Compositor Support is WIP");
    auto c = new ModuleReference;
    c->name = name;
    return c;
}

CompilationUnit Parser::makeAst(vector<Token> &toks)
{
    CompilationUnit unit;
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
            CompoundIdentifier* alias = target;
            if(importExprToks.size()){
                auto as = popf(importExprToks);
                expects(as, TT_as);
                if(importExprToks.size() == 0)error("Expected An Import Alias at " + as.afterpos());
                expects(importExprToks[0], TT_identifier);
                alias = new CompoundIdentifier(popf(importExprToks).value);
                if(importExprToks.size())error("An Import Alias may only be a single identifier");
            }
            unit.push(new Import(target, isStd, alias));
            // Import Path foo.bar.baz
            break;
        }
        case TT_module:
        {
            auto name = popf(toks);
            vector<ModuleReference*> compositors;
            if (toks[0].is(TT_is))
            {
                popf(toks);
                auto compositorToks = readUntil(toks, TT_l_brace, false);
                auto csv = parseCommaSeparatedValues(compositorToks);
                for(auto member:csv){
                    compositors.push_back(parseCompositor(member));
                }
            }
            auto moduleToks = readBlock(toks, IND_braces);
            auto mod = new Module();
            mod->name = new CompoundIdentifier(name.value);
            mod->compositors = compositors;
            parseModuleContent(moduleToks, mod);
            unit.push(mod);
            break;
        }
        default:
            error("Failed to parse Token '" + c.value + "'");
        }
    }
    return unit;
}