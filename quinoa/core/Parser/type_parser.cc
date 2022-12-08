#include "type_parser.hh"
#include "./expression_parser.hh"
#include "./parser_utils.hh"

std::vector<std::shared_ptr<Generic>> parse_generics(std::vector<Token>& toks){
    auto gen_toks = read_block(toks, IND_angles);
    auto gen_entries = parse_cst(gen_toks);

    std::vector<std::shared_ptr<Generic>> generics;

    for(auto ge : gen_entries){
        auto gen_name = pope(ge, TT_identifier).value;

        auto gen = Generic::get(std::make_unique<Name>(gen_name));
        if(ge.size()){
            pope(ge, TT_colon);
            auto constraint = parse_type(ge);
            gen->constraint = constraint;
        }

        generics.push_back(gen);
    }

    return generics;
}

std::vector<std::shared_ptr<Type>> parse_type_args(std::vector<Token>& toks, bool is_fish){
    auto ta_block = is_fish ? read_block(toks, IND_generics) : read_block(toks, IND_angles);
    auto entries = parse_cst(ta_block);

    std::vector<std::shared_ptr<Type>> type_args;
    for(auto e : entries){
        auto ty = parse_type(e);
        type_args.push_back(ty);
    }
    return type_args;
}


std::shared_ptr<Type> parse_type(std::vector<Token>& toks, Container* container)
{
    if (!toks.size())
        except(E_BAD_TYPE, "Failed to parse type");
    std::shared_ptr<Type> ret;
    auto first = popf(toks);
    if(first.is(TT_string)){
        ret = DynListType::get(Primitive::get(PR_uint8));
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
        ret = TypeRef::get(std::move(name));
        if(toks[0].is(TT_lesser)){
            auto type_args = parse_type_args(toks, false);
            ret = ParameterizedTypeRef::get(ret, type_args);
        }

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
