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
        if (d->ttype ==t){
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
inline Token pope(std::vector<Token>& toks, TokenType typ){
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
#include "../AST/advanced_operators.hh"
#include "../AST/constant.hh"
std::unique_ptr<LongName> parse_long_name(std::vector<Token>& toks){
    auto name = std::make_unique<LongName>();

    bool expects_split = false;
    while(toks.size()){
        auto current = toks[0];
        if( expects_split && !current.is(TT_double_colon) )break;
        else if( expects_split ){
            expects_split = false;
            popf(toks);
            continue;
        }
        else{
            popf(toks);
            // 100% has to be an identifier
            expects(current, TT_identifier);
            name->parts.push(Name(current.value));
            expects_split = true;
        }

    }
    return name;
}
std::pair<std::vector<TokenType>, std::vector<TokenType>> get_indenters(){
    std::vector<TokenType> indt;
    std::vector<TokenType> dindt;
    for(auto d : defs) {
	if(d->ind)
	    indt.push_back(d->ttype);
	else if(d->dind)
	    dindt.push_back(d->ttype);
    }
    return {indt, dindt};
}

std::vector<std::vector<Token>> parse_cst(std::vector<Token>& toks){
    if(toks.size() == 0)return {};

    static auto [indt, dindt] = get_indenters();

    std::vector<std::vector<Token>> retVal;
    std::vector<Token> temp;
    int ind = 0;
    while(toks.size()) {
	auto t = popf(toks);
	if(includes(indt, t.type))
	    ind++;
	if(includes(dindt, t.type))
	    ind--;

	if(ind == 0 && t.is(TT_comma)) {
	    retVal.push_back(temp);
	    temp.clear();
	} else {
	    temp.push_back(t);
	}
    }
    retVal.push_back(temp);
    return retVal;
}
std::unique_ptr<Type> parse_type(std::vector<Token>& toks){
    if(!toks.size())except(E_BAD_TYPE, "Failed to parse type");
    std::unique_ptr<Type> ret;

    // string is an alias for 'i8*'
    if( toks[0].is(TT_string)){
        ret = Ptr::get(Primitive::get(PR_int8));
    }
    else if( toks[0].isTypeTok() ){
        auto internal_type = primitive_mappings[popf(toks).type];
        ret = Primitive::get(internal_type);
    }
    else if( toks[0].is(TT_identifier) ){
        except(E_INTERNAL, "Reference Types are currently unsupported");
    }
    return ret;
}
Import parse_import(std::vector<Token> toks){

    Import imp;

    if( toks[0].is(TT_at_symbol) ){
        popf(toks);
        imp.is_stdlib = true;
    }

    imp.target = *parse_long_name(toks);

    if(toks.size()){
        expects(popf(toks), TT_as);
        auto alias_tok = popf(toks);
        expects(alias_tok, TT_identifier);
        imp.alias = alias_tok.value;
    }
    else{
        // The default name is the last part of the import path
        // i.e
        // import a::b::c::d;
        // the default alias would be 'd'
        imp.alias = imp.target.parts[imp.target.parts.len() - 1];
    }
    return imp;

}
ContainerRef parse_container_ref(std::vector<Token> toks){
    auto ref_name = parse_long_name(toks);
    except(E_INTERNAL, "Container Ref Parsing Not Implemented");
}
Param parse_param(std::vector<Token> toks){
    auto param_name = pope(toks, TT_identifier);
    pope(toks, TT_colon);
    auto param_type = parse_type(toks);
    if(toks.size()){
        print_toks(toks);
        except(E_BAD_PARAMETER, "Unexpected Additional Tokens");
    }
    Param param;
    param.name = param_name.value;
    param.type = std::move(param_type);
    return param;
}

struct Name_Segment{
    std::string name;
    Vec<Type>   type_args;
};
Name_Segment parse_name_segment(std::vector<Token>& toks)
{
    auto name = pope(toks, TT_identifier);
    Name_Segment seg;
    seg.name = name.value;

    Vec<Type> generic_args;

    if(toks[0].is(TT_lesser)) {
        auto block = read_block(toks, IND_angles);
        auto cst = parse_cst(block);
        for(auto typ : cst) {
            auto type = parse_type(typ);
            generic_args.push(std::move(type));
        }
    }
    seg.type_args = generic_args;
    return seg;
}
Vec<Name_Segment> parse_segmented_name(std::vector<Token>& toks)
{
    Vec<Name_Segment> segs;
    while(toks.size()) {
        auto id = parse_name_segment(toks);
        segs.push(id);
        if(!toks[0].is(TT_double_colon))break;
        popf(toks);
    }
    return segs;
}
std::unique_ptr<ContainerMemberRef> parse_member_ref_from_segments(Vec<Name_Segment> segments){
    auto end = segments[segments.len() - 1];
    Vec<Name_Segment> container_name_segs;

    for(size_t i = 0; i<segments.len() - 1; i++){
        container_name_segs.push(segments[i]);
    }

    Vec<Type> container_type_args;
    LongName  container_name;
    size_t i = 0;
    for(auto seg : container_name_segs){
        container_name.parts.push(Name(seg.name));

        // Take the last segment generics to be the module type params
        if( i == container_name_segs.len() - 1){
            container_type_args = seg.type_args;
        }
        // No other segment should have type args
        else if(seg.type_args.len()){
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
std::unique_ptr<Expr> parse_expr(std::vector<Token> toks, Scope* parent = nullptr){
    if(!toks.size())except(E_BAD_EXPRESSION, "Cannot generate an expression from 0 tokens");
    
    if(toks.size() == 1){
        auto first = toks[0];
        switch(popf(toks).type){
            case TT_literal_int:return Integer::get(std::stoull(first.value));
            case TT_literal_str:return String::get(first.value);
            default: except(E_BAD_EXPRESSION, "Failed to generate literal for '"+first.value+"'");
        }
    }
    print_toks(toks);

    auto first = toks[0];
    // List Literal
    if(first.is(TT_l_square_bracket)) {
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
    if(first.is(TT_l_paren)){

        auto before = toks;
        auto block = read_block(toks, IND_parens);

        if(!toks.size())return parse_expr(block, parent);

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
    if(first.is(TT_identifier)){
        auto before = toks;
        auto segments = parse_segmented_name(toks);

        auto generic_args = segments[segments.len() - 1].type_args;
        auto member_ref   = parse_member_ref_from_segments(segments);

        if(toks[0].is(TT_l_paren)){
            Vec<Expr> params;
            auto params_block = read_block(toks, IND_parens);
            auto params_cst   = parse_cst(params_block);
            for(auto param_toks : params_cst){
                auto param = parse_expr(param_toks);
                params.push(std::move(param));
            }

            auto call = std::make_unique<MethodCall>();
            call->args = params;
            call->name = std::move(member_ref);
            call->type_args = generic_args;
            return call;

        }

        print_toks(toks);
        toks = before;
    }

    except(E_BAD_EXPRESSION, "Failed to parse expression");
}

std::unique_ptr<Scope> parse_scope(std::vector<Token> toks, Scope* parent = nullptr){
    auto scope = std::make_unique<Scope>();
    scope->scope = parent;
    while(toks.size()){

        auto current = popf(toks);

        if(current.is(TT_while)){
            except(E_INTERNAL, "While loop parser is unimplemented");
        }
        if(current.is(TT_for)){
            except(E_INTERNAL, "For loop parser is unimplemented");
        }
        if(current.is(TT_if)){
            except(E_INTERNAL, "Conditional parser is unimplemented");
        }
        if(current.is(TT_l_brace)){
            except(E_INTERNAL, "Nested scope parser is unimplemented");
        }

        pushf(toks, current);
        // Interpret single-line statements
        auto line = read_to(toks, TT_semicolon);
        popf(toks);

        auto first = popf(line);

        if(first.is(TT_let) || first.is(TT_const)){
            except(E_INTERNAL, "Declaration parser is unimplemented");
        }
        if(first.is(TT_return)){
            auto ret = std::make_unique<Return>();
            ret->value = parse_expr(line, scope.get());
            scope->content.push(std::move(*ret));
            continue;
        }

        // Attempt to interpret the statement as an expression (as a last resort)
        pushf(line, first);
        scope->content.push(parse_expr(line, scope.get()));
    }
    return scope;
}
Vec<ContainerMember> parse_container_content(std::vector<Token>& toks, Container* parent){
    Vec<ContainerMember> ret;

    // bool is_public = false;
    // bool is_inst   = false;

    while(toks.size()){
        auto current = popf(toks);

        if(current.is(TT_func)){
            auto nameTok = popf(toks);
	        expects(nameTok, TT_identifier);

            auto method = std::make_unique<Method>();
            method->parent = parent;


            // Generic Parameters
            if(toks[0].is(TT_lesser)){
                auto gp_block = read_block(toks, IND_angles);
                except(E_INTERNAL, "Method Generic Parameters are unimplemented");
            }

            // Value Parameters
            expects(toks[0], TT_l_paren);
            auto params_block = read_block(toks, IND_parens);
            auto params_sets  = parse_cst(params_block);
            for(auto set : params_sets){
                auto param = parse_param(set);
                method->parameters.push(std::move(param));
            }

            // Return Type
            if(toks[0].is(TT_arrow)){
                popf(toks);
                method->return_type = parse_type(toks);
            }
            else{
                method->return_type = Primitive::get(PR_void);
            }

            // Actual Content
            auto content_toks = read_block(toks, IND_braces);
            auto content = parse_scope(content_toks);
        }    
    }
    return ret;
}

std::unique_ptr<Container> parse_container(std::vector<Token>& toks){
    auto cont = std::make_unique<Container>();

    auto cont_name = pope(toks, TT_identifier);
    cont->name = std::make_unique<Name>(cont_name.value);

    // Seed Compositors
    if( toks[0].is(TT_is) ){
        popf(toks);
        auto sep = parse_cst(toks);
        for(auto entry : sep){
            auto ref = parse_container_ref(entry);
        }
    }

    auto content = read_block(toks, IND_braces);
    auto parsed_content = parse_container_content(content, cont.get());
    cont->members = std::move(parsed_content);

    // If there are leftover tokens in the content, something went wrong
    if(content.size()){
        print_toks(content);
        except(E_INTERNAL, "Failed to parse container content");
    }

    return cont;
}
std::unique_ptr<CompilationUnit> Parser::make_ast(std::vector<Token>& toks){
    
    auto unit = std::make_unique<CompilationUnit>();
    
    while(toks.size()){
        auto current = popf(toks);

        switch(current.type){

            case TT_import:{
                auto expr_toks = read_to(toks, TT_semicolon);
                popf(toks);
                unit->members.push( parse_import( expr_toks ) );
                break;
            }
            case TT_module:
            {
                auto container = parse_container(toks);
                break;
            }
            default: expects(current, TT_notok);

        }
    }
    return unit;
}