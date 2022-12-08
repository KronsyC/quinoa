#include "../../lib/vec.hh"
#include "../../lib/list.h"
#include "../Lexer/lexer.h"
#include "./parser_utils.hh"
#include "./parser.h"
#include "./expression_parser.hh"
#include "./type_parser.hh"

template <typename T, typename U = Statement>
inline std::unique_ptr<U> stm(std::unique_ptr<T> mem)
{
    static_assert(std::is_base_of<U, T>(), "Cannot cast non-statement derivative to statement");
    auto &casted = *(std::unique_ptr<U> *)&mem;
    return std::move(casted);
}

#define st(arg) stm(std::move(arg))








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
                TT_lesser
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
            if (toks[0].is(TT_lesser))
            {
                auto generics = parse_generics(toks);
                method->generic_params = generics;
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
            expects(toks[0], {TT_assignment, TT_lesser});

            std::vector<std::shared_ptr<Generic>> generic_args;
            if(toks[0].is(TT_lesser)){
                generic_args = parse_generics(toks);
            }

            pope(toks, TT_assignment);
            auto type_toks = read_to(toks, TT_semicolon);
            popf(toks);
            auto type = parse_type(type_toks, parent);
            auto member = std::make_unique<TypeMember>(type);
            member->generic_args = generic_args;
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
