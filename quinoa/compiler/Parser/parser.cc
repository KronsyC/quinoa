#include "../../lib/vec.hh"
#include "../../lib/list.h"
#include "../../lib/logger.h"
#include "../AST/ast.hh"
#include "../Lexer/lexer.h"
#include "../token/token.h"
#include <map>
#include <string>

/**
 *
 * Define Various Helper Functions, very useful
 * in the parsing process
 *
 */

std::string get_tt_name(TokenType t)
{
    for (auto d : defs)
    {
        if (d->ttype == t)
        {
            return d->alias.size() ? d->alias[0] : d->name;
        }
        return d->name;
    }
    return "unknown";
}

void print_toks(std::vector<Token> toks)
{
    std::string output;
    for (auto t : toks)
    {
        output += "\t> ";
        output += get_tt_name(t.type) + " -> " + t.value;
        output += "\n";
    }

    Logger::debug("TOKEN DUMP:\n" + output);
}

void expects(Token tok, std::initializer_list<TokenType> expected_types)
{
    bool is_good = false;
    for (auto tt : expected_types)
    {
        if (tok.is(tt))
        {
            is_good = true;
            break;
        }
    }

    if (!is_good)
    {
        std::string message = "Unexpected " + get_tt_name(tok.type) + " '" + tok.value + "'.";
        message += "\n\tWas Expecting: ";
        bool first = true;
        for (auto t : expected_types)
        {
            if (!first)
                message += " | ";
            message += get_tt_name(t);
            first = false;
        }
        except(E_UNEXPECTED_TOKEN, message);
    }
}
void expects(Token tok, TokenType expected_type)
{
    expects(tok, {expected_type});
}
//
// pop-with-error
// pops a token from the list, and errors if it is not a specific type
//
inline Token pope(std::vector<Token> &toks, TokenType typ)
{
    auto tok = popf(toks);
    expects(tok, typ);
    return tok;
}
std::vector<Token> read_to(std::vector<Token> &toks, TokenType type)
{
    std::vector<Token> ret;
    while (toks.size())
    {
        if (toks[0].is(type))
        {
            break;
        }
        ret.push_back(popf(toks));
    }
    return ret;
}

enum IndType
{
    IND_angles,
    INDENTATION_TYPES
};

std::map<IndType, std::pair<TokenType, TokenType>> mappings{{IND_angles, {TT_lesser, TT_greater}}, INDENTATION_MAPPINGS};

std::vector<Token> read_block(std::vector<Token> &toks, IndType typ)
{
    auto p = mappings[typ];
    auto i = p.first;
    auto u = p.second;
    auto first_tok = popf(toks);
    expects(first_tok, i);

    int ind = 1;
    std::vector<Token> ret;
    while (toks.size())
    {
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
        error("Expected corresponding '" + get_tt_name(u) + "' to " + get_tt_name(first_tok.type) + " at " + first_tok.afterpos());
    return ret;
}

/**
 *
 * Actual Parsing Logic Starts Here
 *
 */
#include "./parser.h"
#include "../AST/type.hh"
#include "../AST/symbol_operators.hh"
#include "../AST/advanced_operators.hh"
#include "../AST/constant.hh"
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
std::pair<std::vector<TokenType>, std::vector<TokenType>> get_indenters()
{
    std::vector<TokenType> indt;
    std::vector<TokenType> dindt;
    for (auto d : defs)
    {
        if (d->ind)
            indt.push_back(d->ttype);
        else if (d->dind)
            dindt.push_back(d->ttype);
    }
    return {indt, dindt};
}

std::vector<std::vector<Token>> parse_cst(std::vector<Token> &toks)
{
    if (toks.size() == 0)
        return {};

    static auto [indt, dindt] = get_indenters();

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
std::unique_ptr<Type> parse_type(std::vector<Token> &toks)
{
    if (!toks.size())
        except(E_BAD_TYPE, "Failed to parse type");
    std::unique_ptr<Type> ret;
    auto first = popf(toks);
    // string is an alias for 'i8*'
    if (first.is(TT_string))
    {
        ret = Ptr::get(Primitive::get(PR_int8));
    }
    else if (first.isTypeTok())
    {
        auto internal_type = primitive_mappings[first.type];
        ret = Primitive::get(internal_type);
    }
    else if (first.is(TT_identifier))
    {
        except(E_INTERNAL, "Reference Types are currently unsupported");
    }
    else
        except(E_BAD_TYPE, "Failed to parse type: " + first.value);

    while (toks.size() && toks[0].is(TT_star))
    {
        popf(toks);
        ret = Ptr::get(std::move(ret));
    }
    if(toks[0].is(TT_l_square_bracket)){
        popf(toks);
        pope(toks, TT_r_square_bracket);
        // TODO: possible explicit list length

        ret = ListType::get(std::move(ret));
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

    imp.target = *parse_long_name(toks);
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
        imp.alias = imp.target;
    }
    return imp;
}
ContainerRef parse_container_ref(std::vector<Token> toks)
{
    ContainerRef ref;
    ref.name = parse_long_name(toks);

    if (toks[0].is(TT_lesser))
    {
        auto gen_block = read_block(toks, IND_angles);
        auto gen_cst = parse_cst(gen_block);
        for (auto gen_toks : gen_cst)
        {
            ref.generic_args.push(parse_type(gen_toks));
        }
    }

    return ref;
}
Param parse_param(std::vector<Token> toks)
{
    bool is_variadic = false;
    if(toks[0].is(TT_ellipsis)){
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

struct Name_Segment
{
    std::string name;
    Vec<Type> type_args;
};
Name_Segment parse_name_segment(std::vector<Token> &toks)
{
    auto name = pope(toks, TT_identifier);
    Name_Segment seg;
    seg.name = name.value;

    Vec<Type> generic_args;

    if (toks[0].is(TT_lesser))
    {
        auto block = read_block(toks, IND_angles);
        auto cst = parse_cst(block);
        for (auto typ : cst)
        {
            auto type = parse_type(typ);
            generic_args.push(std::move(type));
        }
    }
    seg.type_args = generic_args;
    return seg;
}
Vec<Name_Segment> parse_segmented_name(std::vector<Token> &toks)
{
    Vec<Name_Segment> segs;
    while (toks.size())
    {
        auto id = parse_name_segment(toks);
        segs.push(id);
        if (!toks[0].is(TT_double_colon))
            break;
        popf(toks);
    }
    return segs;
}
std::unique_ptr<ContainerMemberRef> parse_member_ref_from_segments(Vec<Name_Segment> segments)
{
    auto end = segments[segments.len() - 1];
    Vec<Name_Segment> container_name_segs;

    for (size_t i = 0; i < segments.len() - 1; i++)
    {
        container_name_segs.push(segments[i]);
    }

    Vec<Type> container_type_args;
    LongName container_name;
    size_t i = 0;
    for (auto seg : container_name_segs)
    {
        container_name.parts.push(Name(seg->name));

        // Take the last segment generics to be the module type params
        if (i == container_name_segs.len() - 1)
        {
            container_type_args = seg->type_args;
        }
        // No other segment should have type args
        else if (seg->type_args.len())
        {
            except(E_BAD_ARGS, "Cannot pass generic arguments to a non-module/seed");
        }
        i++;
    }
    auto member_ref = std::make_unique<ContainerMemberRef>();
    member_ref->member = std::make_unique<Name>(end.name);
    member_ref->container = std::make_unique<ContainerRef>();
    member_ref->container->generic_args = container_type_args;
    member_ref->container->name = std::make_unique<LongName>(container_name);
    return member_ref;
}
std::unique_ptr<Expr> parse_expr(std::vector<Token> toks, Scope *parent = nullptr)
{
    if (!toks.size())
        except(E_BAD_EXPRESSION, "Cannot generate an expression from 0 tokens");

    if (toks.size() == 1)
    {
        auto first = toks[0];
        switch (popf(toks).type)
        {
        case TT_literal_int:
            return Integer::get(std::stoull(first.value));
        case TT_literal_str:
            return String::get(first.value);
        case TT_identifier:
            return std::make_unique<SourceVariable>(first.value);
        default:
            except(E_BAD_EXPRESSION, "Failed to generate literal for '" + first.value + "'");
        }
    }

    auto first = toks[0];
    // List Literal
    if (first.is(TT_l_square_bracket))
    {
        auto content = read_block(toks, IND_square_brackets);
        auto entries = parse_cst(content);

        except(E_INTERNAL, "Array literal parser is not implemented");
        // for(auto entry : entries) {
        //     auto entry_expr = parse_expr(entry, parent);
        //     list->push_back(entry_expr);
        // }
        // return list;
    }

    // Unwrap Nested Expression
    if (first.is(TT_l_paren))
    {

        auto before = toks;
        auto block = read_block(toks, IND_parens);

        // if there are leftover tokens, something like
        // (a + b) - c
        // may have been accidentally picked up, skip
        if (!toks.size())
            return parse_expr(block, parent);

        toks = before;
    }

    // Create a function call
    // Function Calls can come in many forms
    //
    // foo::bar();
    // foo<T>::bar();
    // foo::bar<T>();
    // foo<T>::bar<U>();
    // foo::bar<T>::baz();
    // foo::bar<T>::baz<U>();
    // foo::bar::baz<T>();
    //
    // Uses a segmenting approach to simplify
    // the parsing process
    //
    if (first.is(TT_identifier))
    {
        auto before = toks;
        auto segments = parse_segmented_name(toks);

        auto generic_args = segments[segments.len() - 1].type_args;
        auto member_ref = parse_member_ref_from_segments(segments);

        if (toks[0].is(TT_l_paren))
        {
            Vec<Expr> params;
            auto params_block = read_block(toks, IND_parens);
            auto params_cst = parse_cst(params_block);
            for (auto param_toks : params_cst)
            {
                auto param = parse_expr(param_toks);
                params.push(std::move(param));
            }

            auto call = std::make_unique<MethodCall>();
            call->args = params;
            call->name = std::move(member_ref);
            call->type_args = generic_args;
            call->scope = parent;
            return call;
        }

        toks = before;
    }

    // Build a Table of Operator Precedences
    static std::map<TokenType, int> precedences;
    if (!precedences.size())
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
        weight_map.push_back({i, weight});
    }

    // find the best place to split the expression
    int splitPoint = -1;
    int splitWeight = 0;
    for (unsigned int i = 0; i < weight_map.size(); i++)
    {
        auto pair = weight_map[i];
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
            return std::make_unique<UnaryOperation>(std::move(expr), pfxop);
        }
        else if (includes(postfix_ops, toks[toks.size() - 1].type))
        {
            auto op = toks[toks.size() - 1];
            toks.pop_back();
            auto postop = postfix_op_mappings[op.type];
            auto expr = parse_expr(toks, parent);
            return std::make_unique<UnaryOperation>(std::move(expr), postop);
        }

        auto f = toks[0];

        // subscript access
        if (f.is(TT_identifier))
        {
            auto initial = toks;
            auto name = parse_long_name(toks);
            if (toks[0].is(TT_l_square_bracket))
            {
                auto index_block = read_block(toks, IND_square_brackets);
                if (toks.size() == 0)
                {
                    auto var = std::make_unique<SourceVariable>(*name);
                    auto index_expr = parse_expr(index_block, parent);
                    auto subs = std::make_unique<Subscript>(std::move(var), std::move(index_expr));
                    return subs;
                }
            }
            toks = initial;
        }

        print_toks(toks);
        except(E_BAD_EXPRESSION, "Failed To Parse Expression");
    }
    auto op = toks[splitPoint];
    auto [left, right] = split(toks, splitPoint);
    auto leftAST = parse_expr(left, parent);
    auto rightAST = parse_expr(right, parent);

    auto optype = binary_op_mappings[op.type];

    auto binop = std::make_unique<BinaryOperation>(std::move(leftAST), std::move(rightAST), optype);
    // if (optype == BIN_dot)
    // {
    //     if (auto call = dynamic_cast<MethodCall *>(rightAST))
    //     {
    //         call->inst = true;
    //     }
    // }
    // binop->ctx = ctx;
    return binop;
    except(E_INTERNAL, "bad expression");
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
            continue;
            except(E_INTERNAL, "While loop parser is unimplemented");
        }
        if (current.is(TT_for))
        {
            except(E_INTERNAL, "For loop parser is unimplemented");
        }
        if (current.is(TT_if))
        {
            except(E_INTERNAL, "Conditional parser is unimplemented");
        }
        if (current.is(TT_l_brace))
        {
            pushf(toks, current);
            auto nested_toks = read_block(toks, IND_braces);
            auto nested_scope = parse_scope(nested_toks, scope.get());
            scope->content.push(*nested_scope);
            continue;
        }

        // Interpret single-line statements
        auto line = read_to(toks, TT_semicolon);
        popf(toks);

        if (current.is(TT_let) || current.is(TT_const))
        {
            auto init = std::make_unique<InitializeVar>();

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
            ret->value = parse_expr(line, scope.get());
            scope->content.push(std::move(*ret));
            continue;
        }
        pushf(line, current);
        // Attempt to interpret the statement as an expression (as a last resort)
        scope->content.push(parse_expr(line, scope.get()));
    }
    return scope;
}
Vec<ContainerMember> parse_container_content(std::vector<Token> &toks, Container *parent)
{
    Vec<ContainerMember> ret;

    // bool is_public = false;
    // bool is_inst   = false;

    while (toks.size())
    {
        auto current = popf(toks);

        if (current.is(TT_func))
        {
            auto nameTok = popf(toks);
            expects(nameTok, TT_identifier);

            auto method = std::make_unique<Method>();
            method->parent = parent;

            // Generic Parameters
            if (toks[0].is(TT_lesser))
            {
                auto gp_block = read_block(toks, IND_angles);
                except(E_INTERNAL, "Method Generic Parameters are unimplemented");
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
            if(!toks[0].is(TT_semicolon)){
                auto content_toks = read_block(toks, IND_braces);
                auto content = parse_scope(content_toks);
            }
            else popf(toks);

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
        {
            auto container = parse_container(toks);
            unit->members.push(std::move(container));
            break;
        }
        default:
            expects(current, TT_notok);
        }
    }
    return unit;
}