#include "expression_parser.hh"
#include "./parser_utils.hh" 
#include "./type_parser.hh"
std::unique_ptr<ConstantValue> parse_const(Token tok){
    switch (tok.type)
    {
        case TT_literal_int:
            return Integer::get(std::stoll(tok.value));
        case TT_literal_str:
            return String::get(tok.value);
        case TT_literal_false:return Boolean::get(false);
        case TT_literal_true:return Boolean::get(true);
        case TT_literal_float: return Float::get(std::stold(tok.value));
        default:
            except(E_BAD_EXPRESSION, "Failed to generate literal for '" + tok.value + "' with an id of: "+ std::to_string(tok.type));
    }
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
            if(!type_toks.size()){

            toks.pop_back();
            auto castee = parse_expr(toks, parent);

            auto cast = std::make_unique<ExplicitCast>();
            cast->cast_to = cast_to;
            cast->value = std::move(castee);
            return cast;
            }
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

    // function call

    if (first.is(TT_identifier))
    {

        auto before = toks;
        if (toks.size() > 1 && (toks[1].is(TT_op_generic) || toks[1].is(TT_double_colon) || toks[1].is(TT_l_paren)))
        {
            auto container_name = parse_long_name(toks);
            auto member_name = container_name->parts.pop();
            auto member_ref = std::make_unique<ContainerMemberRef>();

            TypeVec type_args;
            if(toks[0].is(TT_op_generic)){
                type_args = parse_type_args(toks);
            }
            member_ref->member = std::make_unique<Name>(member_name);
            if(container_name->parts.len()){
                member_ref->container = std::make_unique<ContainerRef>();
                member_ref->container->name = std::move(container_name);

            }
            if (toks[0].is(TT_l_paren))
            {
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
        }

        toks = before;
    }

    // struct initializations
    if(first.is(TT_identifier)){
        auto before = toks;

        if(toks.size() && (toks[1].is(TT_double_colon) || toks[1].is(TT_op_generic) || toks[1].is(TT_l_brace))){
            auto name = parse_long_name(toks);
            TypeVec generic_args;
            if(toks[0].is(TT_op_generic)){
                generic_args = parse_type_args(toks, true);
            }
            if(toks[0].is(TT_l_brace)){
                
                _Type typ = TypeRef::get(std::move(name));

                if(generic_args.size())typ = ParameterizedTypeRef::get(typ, generic_args);

                auto si = std::make_unique<StructInitialization>(typ);

                auto init_block = read_block(toks, IND_braces);
                if(init_block.size()){
                    expects(init_block[init_block.size() - 1], TT_semicolon);
                    init_block.pop_back();

                    auto entries = parse_tst(init_block, TT_semicolon);


                    for(auto e : entries){
                        auto name = pope(e, TT_identifier);
                        pope(e, TT_colon);
                        auto init = parse_expr(e, parent);
                        init->scope = parent;
                        si->initializers[name.value] = std::move(init);
                    }
                }

                si->scope = parent;
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
    int gen_depth = 0;
    bool decr_gedepth_on_next_iter = false;
    for (unsigned int i = 0; i < toks.size(); i++)
    {
        auto t = toks[i];
        if(decr_gedepth_on_next_iter){
          gen_depth--;
          decr_gedepth_on_next_iter = false;
        }
        if (t.isIndentationTok())
            depth++;
        if (t.isDeIndentationTok())
            depth--;

        if(t.is(TT_op_generic))gen_depth++;
        if(gen_depth && t.is(TT_greater))decr_gedepth_on_next_iter = true;

        if (depth + gen_depth > 0)
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

        // Construct prefix operations info
        std::vector<TokenType> prefix_ops;
        std::vector<TokenType> postfix_ops;
        for (auto def : defs)
        {
            if (def->prefix)
                prefix_ops.push_back(def->ttype);
            if (def->postfix)
                postfix_ops.push_back(def->ttype);
        }

        // Handle Prefix operators
        if (includes(prefix_ops, toks[0].type))
        {
            auto op = popf(toks);
            auto expr = parse_expr(toks, parent);
            auto pfxop = prefix_op_mappings[op.type];
            auto uo = std::make_unique<UnaryOperation>(std::move(expr), pfxop);
            uo->scope = parent;
            return uo;
        }
        // Handle postfix operators
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
        else if(toks[0].is_intrinsic()){
            auto intrinsic_type = intrinsic_mappings[popf(toks).value];

            TypeVec type_args;
            if(toks[0].is(TT_op_generic)){
                type_args = parse_type_args(toks);
            }
            expects(toks[0], TT_l_paren);

            auto params_toks = read_block(toks, IND_parens);
            auto params_entries = parse_cst(params_toks);

            Vec<Expr> params;
            for(auto e : params_entries){
                auto param = parse_expr(e, parent);
                params.push(std::move(param));
            }

            assert(toks.size() == 0);

            return Intrinsic<intr_mul>::create(intrinsic_type, std::move(params), type_args);

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
