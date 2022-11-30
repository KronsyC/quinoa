#include "../../lib/vec.hh"
#include "../../lib/list.h"
#include "../Lexer/lexer.h"
#include "./parser_utils.hh"
#include "./parser.h"
#include "../AST/symbol_operators.hh"
#include "../AST/advanced_operators.hh"
#include "../AST/control_flow.hh"

template <typename T, typename U = Statement>
inline std::unique_ptr<U> stm(std::unique_ptr<T> mem)
{
    static_assert(std::is_base_of<U, T>(), "Cannot cast non-statement derivative to statement");
    auto &casted = *(std::unique_ptr<U> *)&mem;
    return std::move(casted);
}

#define st(arg) stm(std::move(arg))


std::unique_ptr<LongName> parse_long_name(std::vector<Token> &toks)
{
    auto name = std::make_unique<LongName>();

    bool expects_split = false;
    while (toks.size())
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
std::vector<std::vector<Token>> parse_cst(std::vector<Token> &toks)
{
    return parse_tst(toks, TT_comma);
}

std::unique_ptr<ConstantValue> parse_const(Token tok){
    Logger::debug("Const: " + tok.value);
    switch (tok.type)
    {
        case TT_literal_int:
            return Integer::get(std::stoll(tok.value));
        case TT_literal_str:
            return String::get(tok.value);
        case TT_literal_false:return Boolean::get(false);
        case TT_literal_true:return Boolean::get(true);
        default:
            except(E_BAD_EXPRESSION, "Failed to generate literal for '" + tok.value + "'");
    }
}

std::shared_ptr<Type> parse_type(std::vector<Token>& toks, Container* container = nullptr)
{
    if (!toks.size())
        except(E_BAD_TYPE, "Failed to parse type");
    std::shared_ptr<Type> ret;
    auto first = popf(toks);
    if(first.is(TT_string)){
        ret = DynListType::get(Primitive::get(PR_int8));
    }
    else if (first.isTypeTok())
    {
        auto internal_type = primitive_mappings[first.type];
        ret = Primitive::get(internal_type);
    }
    else if (first.is(TT_identifier))
    {
        pushf(toks, first);
        auto name = parse_long_name(toks);
        auto rt = TypeRef::get(std::move(name));
        ret = rt;
    }
    else if(first.is(TT_struct)){
        if(!container)except(E_BAD_TYPE, "Struct types may only be declared as members of a container");
        auto struct_content = read_block(toks, IND_braces);
        if(struct_content.size()){
            expects(struct_content[struct_content.size()-1], TT_semicolon);
            struct_content.pop_back();
        }

        auto entries = parse_tst(struct_content, TT_semicolon);

        std::map<std::string, std::shared_ptr<Type>> fields;
        for(auto entry : entries){
            auto field_name = pope(entry, TT_identifier).value;
            pope(entry, TT_colon);
            auto field_type = parse_type(entry);

            fields[field_name] = field_type;
        }
        ret = StructType::get(fields, container);
    }
    else if(first.is(TT_enum)){
        if(!container)except(E_BAD_TYPE, "Enum types may only be declared as members of a container");
        auto enum_content = read_block(toks, IND_braces);
        if(enum_content.size()){
            expects(enum_content[enum_content.size()-1], TT_semicolon);
            enum_content.pop_back();
        }

        auto entries = parse_tst(enum_content, TT_semicolon);
        std::vector<std::string> members;
        for(auto e : entries){
            if(e.size() != 1 || !e[0].is(TT_identifier))except(E_BAD_TYPE, "An Enum member may only contain a single identifier");
            members.push_back(e[0].value);
        }
        ret = EnumType::get(members, container);

    }
    if(!ret)except(E_BAD_TYPE, "Failed to parse type: " + first.value);
    bool run = true;
    while (toks.size() && run)
    {
        auto first = toks[0];
        switch(first.type){
            case TT_star:{
                popf(toks);
                ret = Ptr::get(ret);
                break;
            }
            case TT_l_square_bracket:{
                popf(toks);
                if(toks[0].is(TT_r_square_bracket)){
                    popf(toks);
                    ret = DynListType::get(ret);
                }
                else{
                    auto len_u = parse_const(popf(toks));
                    auto len = dynamic_cast<Integer*>(len_u.get());

                    if(!len)except(E_BAD_ARRAY_LEN, "Arrays must have a constant integer length");
                    pope(toks, TT_r_square_bracket);

                    ret = ListType::get(ret, std::move(std::unique_ptr<Integer>((Integer*)len_u.release())));
                }
                break;

            }
            case TT_ampersand:{
                popf(toks);
                ret = ReferenceType::get(ret);
                break;
            }
            default: run = false;
        }
    }


    return ret;
}
Import parse_import(std::vector<Token> toks)
{

    Import imp;

    if (toks[0].is(TT_at_symbol))
    {
        popf(toks);
        imp.is_stdlib = true;
    }

    imp.target = std::move(*parse_long_name(toks));
    if (toks.size())
    {
        expects(popf(toks), TT_as);
        auto alias_tok = popf(toks);
        expects(alias_tok, TT_identifier);
        imp.alias.parts.push(Name(alias_tok.value));
    }
    else
    {
        // The default name is the import path
        for(auto item : imp.target.parts){
            imp.alias.parts.push(*item.ptr);
        }
    }
    return imp;
}
ContainerRef parse_container_ref(std::vector<Token> toks)
{
    ContainerRef ref;
    ref.name = parse_long_name(toks);

    return ref;
}
Param parse_param(std::vector<Token> toks)
{
    bool is_variadic = false;
    if (toks[0].is(TT_ellipsis))
    {
        is_variadic = true;
        popf(toks);
    }
    auto param_name = pope(toks, TT_identifier);
    pope(toks, TT_colon);
    auto param_type = parse_type(toks);
    if (toks.size())
    {
        print_toks(toks);
        except(E_BAD_PARAMETER, "Unexpected Additional Tokens");
    }
    Param param(param_name.value, std::move(param_type));
    param.is_variadic = is_variadic;
    return param;
}


std::unique_ptr<Expr> parse_expr(std::vector<Token> toks, Scope *parent)
{

    if (!toks.size())
        except(E_INTERNAL, "Cannot generate an expression from 0 tokens");
    if (toks.size() == 1)
    {
        if(toks[0].is(TT_identifier)){
            auto id = std::make_unique<SourceVariable>(toks[0].value);
            id->scope = parent;
            return id;
        }
        else return parse_const(popf(toks));
    }

    auto first = toks[0];

    // Casts `expr as type`
    {
        auto initial = toks;
        auto type_toks = read_to_reverse(toks, TT_as);
        if(type_toks.size()){
            auto cast_to = parse_type(type_toks);
            toks.pop_back();
            auto castee = parse_expr(toks, parent);

            auto cast = std::make_unique<ExplicitCast>();
            cast->cast_to = cast_to;
            cast->value = std::move(castee);
            return cast;
        }
        toks = initial;

    }

    // Long variable name:  Foo::Bar::baz
    if(first.is(TT_identifier)){
        auto before = toks;
        auto name = parse_long_name(toks);
        if(!toks.size()){
            auto var = std::make_unique<SourceVariable>(std::move(name));
            var->scope = parent;
            return var;
        }
        toks = before;
    }


    // List Literal
    if (first.is(TT_l_square_bracket))
    {
        auto initial = toks;
        auto content = read_block(toks, IND_square_brackets);
        if(!toks.size()){
            auto entries = parse_cst(content);
            auto list = std::make_unique<ArrayLiteral>();
            list->scope = parent;
            for(auto entry : entries) {
                auto entry_expr = parse_expr(entry, parent);
                list->members.push(std::move(entry_expr));
            }
            return list;
        }
        toks = initial;
    }

    // Subscript
    if (toks[toks.size()-1].is(TT_r_square_bracket))
    {
        auto initial = toks;
        auto target_toks = read_to(toks, TT_l_square_bracket);
        if(target_toks.size()){
            auto target = parse_expr(target_toks, parent);
            if (toks[0].is(TT_l_square_bracket))
            {
                // subscript access

                auto index_block = read_block(toks, IND_square_brackets);
                if (toks.size() == 0)
                {
                    auto index_expr = parse_expr(index_block, parent);
                    auto subs = std::make_unique<Subscript>(std::move(target), std::move(index_expr));
                    subs->scope = parent;
                    return subs;
                }
            }
        }


        toks = initial;
    }


    // Unwrap Nested Expression
    if (first.is(TT_l_paren))
    {

        auto before = toks;
        auto block = read_block(toks, IND_parens);

        // if there are leftover tokens, something like
        // (a + b) - c
        // may have been accidentally picked up, skip
        if (toks.empty())
            return parse_expr(block, parent);

        toks = before;
    }


    if (first.is(TT_identifier))
    {

        auto before = toks;
        if (toks.size() > 1 && (toks[1].is(TT_l_generic) || toks[1].is(TT_double_colon) || toks[1].is(TT_l_paren) || toks[1].is(TT_l_brace)))
        {
            auto container_name = parse_long_name(toks);
            auto member_name = container_name->parts.pop();
            auto member_ref = std::make_unique<ContainerMemberRef>();

            std::vector<std::shared_ptr<Type>> type_args;
            if(toks[0].is(TT_l_generic)){
                auto gen_block = read_block(toks, IND_generics);
                auto entries = parse_cst(gen_block);
                for(auto e : entries){
                    auto ty = parse_type(e);
                    type_args.push_back(ty);
                }
            }
            member_ref->member = std::make_unique<Name>(member_name);
            if(container_name->parts.len()){
                member_ref->container = std::make_unique<ContainerRef>();
                member_ref->container->name = std::move(container_name);

            }
            if (toks[0].is(TT_l_paren))
            {
                // function call
                Vec<Expr> params;
                auto params_block = read_block(toks, IND_parens);

                if(!toks.size()){
                    auto params_cst = parse_cst(params_block);
                    for (const auto& param_toks : params_cst)
                    {
                        auto param = parse_expr(param_toks, parent);
                        params.push(std::move(param));
                    }

                    auto call = std::make_unique<MethodCall>();
                    call->args = std::move(params);
                    call->type_args = type_args;
                    call->name = std::move(member_ref);
                    call->scope = parent;
                    return call;
                }

            }
            else if(toks[0].is(TT_l_brace)){
                // Struct initialization block

                Logger::debug("struct of type: " + member_ref->str());
                auto init_block = read_block(toks, IND_braces);
                auto si = std::make_unique<StructInitialization>(std::move(member_ref));
                if(!init_block.empty()){
                    expects(init_block[init_block.size()-1], TT_semicolon);
                    init_block.pop_back();

                    auto prop_init_toks = parse_tst(init_block, TT_semicolon);
                    for(auto init : prop_init_toks){
                        auto prop_name = pope(init, TT_identifier).value;
                        pope(init, TT_colon);
                        auto prop_value = parse_expr(init, parent);

                        si->initializers[prop_name] = std::move(prop_value);
                    }
                }

                return si;
            }
        }

        toks = before;
    }

    // Build a Table of Operator Precedences
    static std::map<TokenType, int> precedences;
    if (precedences.empty())
    {
        for (auto d : defs)
        {
            if (d->infix)
            {
                precedences[d->ttype] = d->infix;
            }
        }
    }

    std::vector<std::pair<int, int>> weight_map;
    int depth = 0;
    for (unsigned int i = 0; i < toks.size(); i++)
    {
        auto t = toks[i];

        if (t.isIndentationTok())
            depth++;
        if (t.isDeIndentationTok())
            depth--;

        if (depth > 0)
            continue;

        int weight = precedences[t.type];
        if (weight == 0)
            continue;
        weight_map.emplace_back(i, weight);
    }

    // find the best place to split the expression
    int splitPoint = -1;
    int splitWeight = 0;
    for (auto pair : weight_map)
    {
        int index = pair.first;
        int weight = pair.second;
        if (splitPoint == -1 || (splitWeight <= weight))
        {
            splitPoint = index;
            splitWeight = weight;
        }
    }

    //
    // It is a unary operation if there is either:
    // 1. No split point
    // 2. Split point at index 0
    // 3. Split point at index len-1
    //
    if (splitPoint == -1 || splitPoint == 0 || splitPoint == (long long)toks.size() - 1)
    {

        // Construct prefix ops
        std::vector<TokenType> prefix_ops;
        std::vector<TokenType> postfix_ops;
        for (auto def : defs)
        {
            if (def->prefix)
                prefix_ops.push_back(def->ttype);
            if (def->postfix)
                postfix_ops.push_back(def->ttype);
        }

        if (includes(prefix_ops, toks[0].type))
        {
            auto op = popf(toks);
            auto expr = parse_expr(toks, parent);
            auto pfxop = prefix_op_mappings[op.type];
            auto uo = std::make_unique<UnaryOperation>(std::move(expr), pfxop);
            uo->scope = parent;
            return uo;
        }
        else if (includes(postfix_ops, toks[toks.size() - 1].type))
        {
            auto op = toks[toks.size() - 1];
            toks.pop_back();
            auto postop = postfix_op_mappings[op.type];
            auto expr = parse_expr(toks, parent);
            auto uo = std::make_unique<UnaryOperation>(std::move(expr), postop);
            uo->scope = parent;
            return uo;
        }



        print_toks(toks);
        except(E_BAD_EXPRESSION, "Failed To Parse Expression");
    }
    auto op = toks[splitPoint];
    auto [left, right] = split(toks, splitPoint);
    auto leftAST = parse_expr(left, parent);
    auto rightAST = parse_expr(right, parent);

    auto optype = binary_op_mappings[op.type];

    if(optype == BIN_dot){
        expects(right[0], TT_identifier);
        if (dynamic_cast<MethodCall*>(rightAST.get()))
        {
            auto base_call = std::unique_ptr<MethodCall>((MethodCall*)rightAST.release() );
            if(base_call->name->container)except(E_BAD_EXPRESSION, "Instance calls may only be identified by single-part identifiers");
            auto call = std::make_unique<MethodCallOnType>();
            call->method_name = std::move(base_call->name->member);
            call->args = std::move(base_call->args);
            call->type_args = std::move(base_call->type_args);

            call->scope = base_call->scope;
            call->call_on = std::move(leftAST);
            return call;
        }
        else if(dynamic_cast<SourceVariable*>(rightAST.get())){
            auto base_var = std::unique_ptr<SourceVariable>((SourceVariable*)rightAST.release());
            if(base_var->name->parts.len() > 1)except(E_BAD_MEMBER_ACCESS, "You may only use simple identifiers to access members");
            auto sub = std::make_unique<MemberAccess>(std::move(leftAST), std::make_unique<Name>(base_var->name->last()));
            return sub;
        }
    }


    auto binop = std::make_unique<BinaryOperation>(std::move(leftAST), std::move(rightAST), optype);
    binop->scope = parent;
    return binop;
}

std::unique_ptr<Scope> parse_scope(std::vector<Token> toks, Scope *parent = nullptr)
{
    auto scope = std::make_unique<Scope>();
    scope->scope = parent;
    while (toks.size())
    {

        auto current = popf(toks);

        if (current.is(TT_while))
        {
            auto eval_toks = read_block(toks, IND_parens);
            auto exec_toks = read_block(toks, IND_braces);

            auto eval_expr = parse_expr(eval_toks, scope.get());
            auto exec_scope = parse_scope(exec_toks, scope.get());

            auto loop = std::make_unique<While>();
            loop->execute = std::move(exec_scope);
            loop->condition = std::move(eval_expr);
            loop->scope = scope.get();
            scope->content.push(st(loop));
            continue;
        }
        if (current.is(TT_for))
        {
            except(E_INTERNAL, "For loop parser is unimplemented");
        }
        if (current.is(TT_if))
        {
            auto eval_toks = read_block(toks, IND_parens);
            auto exec_toks = read_block(toks, IND_braces);

            auto eval_expr = parse_expr(eval_toks, scope.get());
            auto exec_scope = parse_scope(exec_toks, scope.get());

            auto cond = std::make_unique<Conditional>();
            cond->condition = std::move(eval_expr);
            cond->if_true = std::move(exec_scope);
            cond->scope = scope.get();

            if (toks[0].is(TT_else))
            {
                popf(toks);
                auto else_toks = read_block(toks, IND_braces);
                auto else_scope = parse_scope(else_toks, scope.get());
                cond->if_false = std::move(else_scope);
            }

            scope->content.push(st(cond));
            continue;
        }
        if (current.is(TT_l_brace))
        {
            pushf(toks, current);
            auto nested_toks = read_block(toks, IND_braces);
            auto nested_scope = parse_scope(nested_toks, scope.get());
            scope->content.push(std::move(*nested_scope));
            continue;
        }

        // Interpret single-line statements
        // The push and pop may seem unnecessary, but they are required as read_to takes
        // indentation into account, which has issues when the line starts with `(`
        pushf(toks, current);
        auto line = read_to(toks, TT_semicolon);
        popf(toks);
        popf(line);
        if (current.is(TT_break))
        {
            scope->content.push(stm(std::make_unique<ControlFlowJump>(JumpType::BREAK)));
            continue;
        }
        if (current.is(TT_continue))
        {
            scope->content.push(stm(std::make_unique<ControlFlowJump>(JumpType::CONTINUE)));
            continue;
        }
        if (current.is(TT_fallthrough))
        {
            scope->content.push(stm(std::make_unique<ControlFlowJump>(JumpType::FALLTHROUGH)));
            continue;
        }

        if (current.is(TT_let) || current.is(TT_const))
        {
            auto init = std::make_unique<InitializeVar>();
            init->scope = scope.get();
            init->var_name = pope(line, TT_identifier).value;
            if (current.is(TT_const))
                init->is_constant = true;

            // Attempt to read a type
            if (line[0].is(TT_colon))
            {
                popf(line);
                auto type = parse_type(line);
                init->type = std::move(type);
            }

            // Attempt to read an initializer value
            if (line[0].is(TT_assignment))
            {
                popf(line);
                auto initializer = parse_expr(line, scope.get());
                init->initializer = std::move(initializer);
            }
            scope->content.push(std::move(*init));
            continue;
        }
        if (current.is(TT_return))
        {
            auto ret = std::make_unique<Return>();
            ret->scope = scope.get();
            if (line.size())
            {
                ret->value = parse_expr(line, scope.get());
            }
            scope->content.push(std::move(*ret));
            continue;
        }
        pushf(line, current);
        // Attempt to interpret the statement as an expression (as a last resort)
        scope->content.push(st(parse_expr(line, scope.get())));
    }
    return scope;
}
Vec<ContainerMember> parse_container_content(std::vector<Token> &toks, Container *parent)
{

    Vec<ContainerMember> ret;

    bool is_public = true;

    Vec<Attribute> attrs;

    while (toks.size())
    {
        auto current = popf(toks);

        if(current.is(TT_hashtag)){
            // Metadata

            auto content = read_block(toks, IND_square_brackets);
            auto attr_name = pope(content, TT_identifier).value;
            Vec<ConstantValue> attr_args;
            while(content.size()){
                auto tok = popf(content);
                auto expr = parse_expr({tok}, nullptr);

                if(dynamic_cast<ConstantValue*>(expr.get())){
                    attr_args.push(std::unique_ptr<ConstantValue>((ConstantValue*)expr.release()));
                }
                else except(E_BAD_ARGS, "Arguments to an attribute must be constants");
            }

            Attribute attr;
            attr.arguments = std::move(attr_args);
            attr.name = attr_name;
            attrs.push(std::move(attr));

        }

        else if(current.is(TT_private) && is_public){
            is_public = false;
        }

        else if (current.is(TT_func))
        {
            auto nameTok = popf(toks);
            expects(nameTok, TT_identifier);

            auto method = std::make_unique<Method>();
            method->parent = parent;
            method->name = std::make_unique<ContainerMemberRef>();

            method->name->member = std::make_unique<Name>(nameTok.value);
            method->name->container = parent->get_ref();

            expects(toks[0], {
                TT_l_paren,
                TT_dot,
                TT_l_generic
            });

            // Acts on specific type
            if(toks[0].is(TT_dot)){
                popf(toks);
                auto acts_on = parse_type(toks);

                // methods may only act upon locally defined types
                if(acts_on->get<TypeRef>()){
                    method->acts_upon = std::static_pointer_cast<TypeRef>(acts_on);
                }
                else except(E_BAD_METHOD_DEFINITION, "A method may only act upon locally defined types");
            }

            // Generic Parameters
            if (toks[0].is(TT_l_generic))
            {
                auto gp_block = read_block(toks, IND_generics);
                auto gp_sets = parse_cst(gp_block);

                for(auto ty : gp_sets){
                    assert(ty.size() == 1);
                    expects(ty[0], TT_identifier);
                    method->generic_params.push_back(Generic::get(std::make_unique<Name>(ty[0].value), method.get()));
                }
                print_toks(gp_block);
            }

            // Value Parameters
            expects(toks[0], TT_l_paren);
            auto params_block = read_block(toks, IND_parens);
            auto params_sets = parse_cst(params_block);
            for (auto set : params_sets)
            {
                auto param = parse_param(set);
                method->parameters.push(std::move(param));
            }

            // Return Type
            if (toks[0].is(TT_arrow))
            {
                popf(toks);
                method->return_type = parse_type(toks);
            }
            else
            {
                method->return_type = Primitive::get(PR_void);
            }

            // Actual Content
            if (!toks[0].is(TT_semicolon))
            {
                auto content_toks = read_block(toks, IND_braces);
                auto content = parse_scope(content_toks);
                method->content = std::move(content);
            }
            else
                popf(toks);

            method->local_only = !is_public;
            method->attrs = std::move(attrs);
            is_public = true;

            ret.push(stm<Method, ContainerMember>(std::move(method)));
        }
        else if(current.is(TT_type)){
            auto type_name = popf(toks).value;
            pope(toks, TT_assignment);
            auto type_toks = read_to(toks, TT_semicolon);
            popf(toks);
            auto type = parse_type(type_toks, parent);
            auto member = std::make_unique<TypeMember>(type);
            member->parent = parent;
            member->name = std::make_unique<ContainerMemberRef>();
            member->name->container = parent->get_ref();
            member->name->member = std::make_unique<Name>(type_name);

            ret.push(stm<TypeMember, ContainerMember>(std::move(member)));
        }
        else if(current.is(TT_identifier)){
            auto prop = std::make_unique<Property>();

            prop->name = std::make_unique<ContainerMemberRef>();
            prop->name->member = std::make_unique<Name>(current.value);
            prop->name->container = parent->get_ref();
            prop->parent = parent;
            auto line = read_to(toks, TT_semicolon);
            popf(toks);
            expects(popf(line), TT_colon);
            prop->type = parse_type(line);

            if(line.size()){
                expects(popf(line), TT_assignment);
                auto init = parse_expr(line, nullptr);
                if(dynamic_cast<ConstantValue*>(init.get())){
                    prop->initializer = std::move(*(std::unique_ptr<ConstantValue>*)&init);
                }
                else except(E_BAD_ASSIGNMENT, "An Initializer must be a constant value");
            }
            prop->local_only = !is_public;
            is_public = true;
            ret.push(stm<Property, ContainerMember>(std::move(prop)));
        }
        else{

            except(E_ERR, "Unexpected Module Member: " + get_tt_name(current.type));
        }
    
    }

    return ret;
}

std::unique_ptr<Container> parse_container(std::vector<Token> &toks)
{
    auto cont = std::make_unique<Container>();

    auto cont_name = pope(toks, TT_identifier);
    cont->name = std::make_unique<Name>(cont_name.value);

    // Seed Compositors
    if (toks[0].is(TT_is))
    {
        popf(toks);
        auto compositor_toks = read_to(toks, TT_l_brace);
        auto sep = parse_cst(compositor_toks);
        for (auto entry : sep)
        {
            auto ref = parse_container_ref(entry);
            cont->compositors.push(std::move(ref));
        }
    }
    auto content = read_block(toks, IND_braces);
    auto parsed_content = parse_container_content(content, cont.get());
    // auto mod = (Module*)&parsed_content[0];
    // Logger::debug("mod:" + mod->name->str());
    cont->members = std::move(parsed_content);

    // If there are leftover tokens in the content, something went wrong
    if (content.size())
    {
        print_toks(content);
        except(E_INTERNAL, "Failed to parse container content");
    }

    return cont;
}
std::unique_ptr<CompilationUnit> Parser::make_ast(std::vector<Token> &toks)
{

    auto unit = std::make_unique<CompilationUnit>();

    while (toks.size())
    {
        auto current = popf(toks);

        switch (current.type)
        {

        case TT_import:
        {
            auto expr_toks = read_to(toks, TT_semicolon);
            popf(toks);
            unit->members.push(parse_import(expr_toks));
            break;
        }
        case TT_module:
        case TT_seed:
        case TT_struct:
        {
            auto container = parse_container(toks);
            container->type = current.is(TT_module) ? CT_MODULE 
                            : current.is(TT_seed)   ? CT_SEED
                            : current.is(TT_struct) ? CT_STRUCT
                            : CT_NOTYPE;
            container->parent = unit.get();
            if(current.is(TT_struct)){
                // instance all members
                for(auto mem : container->members){
                    mem->instance_only = true;
                }
            }

            unit->members.push(stm<Container, TopLevelEntity>(std::move(container)));
            break;
        }
        default:
            expects(current, TT_notok);
        }
    }
    return unit;
}