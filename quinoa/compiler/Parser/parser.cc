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
        
    }
    except(E_INTERNAL, "Type parsing is not yet supported");
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
    except(E_INTERNAL, "Not Implemented");
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