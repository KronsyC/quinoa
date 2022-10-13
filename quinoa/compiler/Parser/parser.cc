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
CompoundIdentifier *parseIdentifier(vector<Token> &toks, SourceBlock* ctx)
{
    expects(toks[0], TT_identifier);
    // Simplest Case (single-part)
    if (toks.size() < 3 || !(toks[1].is(TT_double_colon) && toks[2].is(TT_identifier))){
        return new CompoundIdentifier(popf(toks).value, ctx);
    }
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
            parts.push_back(Ident::get(t.value, ctx));
            expectDot = true;
            continue;
        }
        break;
    }
    for (int i = 0; i < parts.size() * 2 - 1; i++)
        popf(toks);
    auto id = new CompoundIdentifier(parts);
    id->ctx = ctx;
    return id;
}
Expression *parseExpression(vector<Token> &toks, SourceBlock* ctx);
Type* parseType(vector<Token>& toks, SourceBlock* ctx){
    if(!toks.size())error("Failed To Parse Type");
    Type* ret = nullptr;
    // Special case for strings
    if(toks[0].is(TT_string)){
        popf(toks);
        ret = TPtr::get(Primitive::get(PR_int8));
    }
    else if(toks[0].isTypeTok())ret = Primitive::get(primitive_mappings[popf(toks).type]);
    else{
        auto references = parseIdentifier(toks, ctx);
        ret = CustomType::get(references);
    }

    while(toks.size() && toks[0].is(TT_star)){
        popf(toks);
        ret = TPtr::get(ret);
    }
    if(toks.size() && toks[0].is(TT_l_square_bracket)){
        // Type Array
        auto size = readBlock(toks, IND_square_brackets);
        if(size.size()==0)ret=ListType::get(ret, nullptr);
        else ret = ListType::get(ret, parseExpression(size, ctx));
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


Expression *parseExpression(vector<Token> &toks, SourceBlock* ctx)
{
    if (toks.size() == 0)
        error("Cannot Generate an Expression from no tokens");
    if (toks.size() == 1)
    {
        switch (toks[0].type)
        {
        case TT_literal_int:return new Integer(stoull(toks[0].value));
        case TT_literal_true:
            return new Boolean(true);
        case TT_literal_false:
            return new Boolean(false);
        case TT_literal_str: return new String(toks[0].value);
        case TT_literal_float:
            return new Float(stold(toks[0].value));
        case TT_identifier:{
            return Ident::get(toks[0].value, ctx);

        }
        default:
            error("Failed To Generate an Appropriate Constant Value for '" + getTokenTypeName(toks[0].type) + "'");
        }
    }

    auto c = toks[0];

    if (c.is(TT_identifier))
    {
        auto initial = toks;
        auto target = parseIdentifier(toks, ctx);
        target->ctx = ctx;
        if (toks[0].is(TT_l_paren))
        {
            auto params = readBlock(toks, IND_parens);
            if (toks.size() == 0)
            {
                int i = 1; 
                auto paramsList = parseCommaSeparatedValues(params);
                vector<Expression *> params;
                for (auto p : paramsList)
                {
                    auto expr = parseExpression(p, ctx);
                    expr->ctx = ctx;
                    params.push_back(expr);
                }
                if(target->str() == "len" && params.size() == 1 && instanceof<Identifier>(params[0])){
                    auto l = new ArrayLength((Identifier*)params[0]);
                    l->ctx = ctx;
                    return l;
                }
                auto call = new MethodCall;
                call->params = params;
                call->target = nullptr;
                call->ctx = ctx;
                call->name = target;
                


                return call;
            }
        }
        else if(toks[0].is(TT_l_square_bracket)){
            auto etoks = readBlock(toks, IND_square_brackets);
            if(toks.size() == 0){
                auto item = parseExpression(etoks, ctx);
                item->ctx = ctx;
                return new Subscript(target, item);
            }
        }
        toks = initial;
    }

    if(c.is(TT_l_square_bracket)){
        auto content = readBlock(toks, IND_square_brackets);
        auto entries = parseCommaSeparatedValues(content);
        
        auto list = new List;

        for(auto entry:entries){
            auto entryExpr = parseExpression(entry, ctx);
            list->push_back(entryExpr);
        }
        return list;
    }

    if (c.is(TT_l_paren))
    {
        auto initial = toks;
        auto block = readBlock(toks, IND_parens);
        if (toks.size() == 0)
            return parseExpression(block, ctx);

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

    vector<std::pair<int, int>> weightMap;
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
        weightMap.push_back({i, weight});
    }

    // find the best place to split the expression
    int splitPoint = -1;
    int splitWeight = 0;
    for (int i = 0; i < weightMap.size(); i++)
    {
        auto pair = weightMap[i];
        int index = pair.first;
        int weight = pair.second;
        if (splitPoint == -1 || (splitWeight <= weight))
        {
            splitPoint = index;
            splitWeight = weight;
        }
    }
    if (splitPoint == -1 || splitPoint==0 || splitPoint==toks.size()-1)
    {
        // no split, start split or end split should cover all cases


        // Construct prefix ops
        std::vector<TokenType> prefix_ops;
        std::vector<TokenType> postfix_ops;
        for(auto def:defs){
            if(def->prefix)prefix_ops.push_back(def->ttype);
            if(def->postfix)postfix_ops.push_back(def->ttype);
        }

        if(includes(prefix_ops, toks[0].type)){
            auto op = popf(toks);
            auto expr = parseExpression(toks, ctx);
            auto pfxop = prefix_op_mappings[op.type];
            return new UnaryOperation(expr, pfxop);
        }
        else if(includes(postfix_ops, toks[toks.size()-1].type)){
            auto op = toks[toks.size()-1];
            toks.pop_back();
            auto postop = postfix_op_mappings[op.type];
            auto expr = parseExpression(toks, ctx);
            return new UnaryOperation(expr, postop);
        }
        printToks(toks);
        error("Failed To Parse Expression");
    }
    auto op = toks[splitPoint];
    auto [left, right] = split(toks, splitPoint);
    auto l = left;
    auto r = right;
    auto leftAST = parseExpression(left, ctx);
    auto rightAST = parseExpression(right, ctx);
    auto optype = binary_op_mappings[op.type];
    return new BinaryOperation(leftAST, rightAST, optype);
}

SourceBlock* parseSourceBlock(vector<Token> toks, LocalTypeTable typeinfo={})
{
    auto block = new SourceBlock;
    auto type_info = new LocalTypeTable;
    *type_info = typeinfo;
    block->local_types = type_info;
    while (toks.size())
    {
        auto first = toks[0];
        if(first.is(TT_while)){
            popf(toks);
            auto expr = readBlock(toks, IND_parens);
            auto exec = readBlock(toks, IND_braces);
            auto cond = parseExpression(expr, block);
            auto content= parseSourceBlock(exec, *type_info);
            auto loop = new WhileCond;
            cond->ctx = block;
            loop->local_types = new LocalTypeTable;
            loop->insert(content);
            loop->cond = cond;
            loop->ctx = block;
            // For some reason the loop becomes inactive at this point
            loop->active = true;
            block->push_back(loop);
            continue;
        }
        if(first.is(TT_if)){
            popf(toks);
            auto cond = readBlock(toks, IND_parens);
            auto condExpr = parseExpression(cond, block);
            condExpr->ctx = block;

            auto iff = new IfCond;
            iff->cond = condExpr;

            auto does = readBlock(toks, IND_braces);
            auto doesA = parseSourceBlock(does, *type_info);
            if(!doesA)error("Failed to parse conditional block");
            iff->does = doesA;
            if(toks[0].is(TT_else)){
                popf(toks);
                auto otherwise = readBlock(toks, IND_braces);
                iff->otherwise = parseSourceBlock(otherwise, *type_info);
            }
            block->push_back(iff);
            continue;

        }
        if(first.is(TT_for)){
            popf(toks);
            auto inner = readBlock(toks, IND_parens);
            auto exec = readBlock(toks, IND_braces);

            auto init = readUntil(inner, TT_semicolon);
            auto sc = popf(inner);
            init.push_back(sc);
            auto check = readUntil(inner, TT_semicolon, true);
            inner.push_back(sc);
            auto initCode = parseSourceBlock(init, *type_info);
            block->insert(initCode);

            auto checkCode = parseExpression(check, block);
            auto incCode = parseSourceBlock(inner, *type_info);
            auto source = parseSourceBlock(exec, *type_info);
            auto loop = new ForRange;

            loop->cond = checkCode;
            loop->inc = incCode;
            loop->ctx = block;
            loop->local_types = new LocalTypeTable;
            *loop->local_types = *block->local_types;
            loop->insert(source);
            block->push_back(loop);

            continue;
        }
        auto f = toks[0];
        auto line = readUntil(toks, TT_semicolon, true);
        if(line.size()==0)expects(f, TT_notok);
        f = line[0];

        if (f.is(TT_return))
        {
            popf(line);
            auto returnValue = parseExpression(line, block);
            auto ret = new Return(returnValue);
            ret->ctx = block;
            returnValue->ctx = block;
            block->push_back(ret);
            continue;
        }
        else if(f.is(TT_let)){
            popf(line);    
            auto varname = popf(line).value; 
            Type* vartype;
            if(line[0].is(TT_colon)){
                popf(line);
                vartype = parseType(line, block);
            }       
            else vartype = nullptr;
            auto name = Ident::get(varname);
            name->ctx = block;

            // Add the variable to the type table
            (*type_info)[varname] = vartype;
            block->declarations.push_back(varname);
            auto init = new InitializeVar(vartype, name);
            init->ctx = block;
            if(line.size() != 0){
                expects(popf(line), TT_assignment);
                auto val = parseExpression(line, block);
                val->ctx = block;
                init->initializer = val;
            }
            block->push_back(init);
            continue;
        }

        // Default to expression parsing
        auto expr = parseExpression(line, block);
        expr->ctx = block;

        block->push_back(expr);
    }

 


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
    auto type = parseType(toks, nullptr);

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
            auto method = new Method();
            method->local_types = new LocalTypeTable;

            // Functions implicitly return void
            if(toks[0].is(TT_arrow)){
                popf(toks);
                returnType = parseType(toks, method);
            }
            else{
                returnType = Primitive::get(PR_void);
            }

            auto argsCSV = parseCommaSeparatedValues(argsTokens);
            Block<Param> params;
            LocalTypeTable argTypes;
            for(auto a:argsCSV){
                auto param = parseParameter(a);
                argTypes[param->name->str()] = param->type;
                params.push_back(param);
            }
            // keep track of the arg types too
            *method->local_types = argTypes;
            auto sig = new MethodSignature();
            if(toks[0].is(TT_l_brace)){
               auto contentToks = readBlock(toks, IND_braces);
                auto content = parseSourceBlock(contentToks, argTypes);
                method->insert(content);
            }
            else{
                 expects(toks[0], TT_semicolon);
                 popf(toks);
            }

            method->sig = sig;
            sig->name = Ident::get(nameTok.value);
            sig->space = mod->name;

            sig->params = Block<Param>(params.take());
            sig->returnType = returnType;
            mod->push_back(method);
            continue;
            
        }

        error("Functions are the only module members currently supported");
    }
}

ModuleReference* parseCompositor(vector<Token>& toks){
    auto name = parseIdentifier(toks, nullptr);
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
            auto target = parseIdentifier(importExprToks, nullptr);
            CompoundIdentifier* alias = target;
            if(importExprToks.size()){
                auto as = popf(importExprToks);
                expects(as, TT_as);
                if(importExprToks.size() == 0)error("Expected An Import Alias at " + as.afterpos());
                expects(importExprToks[0], TT_identifier);
                alias = new CompoundIdentifier(popf(importExprToks).value);
                if(importExprToks.size())error("An Import Alias may only be a single identifier");
            }
            unit.push_back(new Import(target, isStd, alias));
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
            unit.push_back(mod);
            break;
        }
        default:
            error("Failed to parse Token '" + c.value + "'");
        }
    }
    return unit;
}