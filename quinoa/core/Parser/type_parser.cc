#include "type_parser.hh"
#include "./expression_parser.hh"
#include "./parser_utils.hh"
#include <memory>

std::vector<std::shared_ptr<Generic>> parse_generics(std::vector<Token>& toks) {
    auto gen_toks = read_block(toks, IND_angles);
    auto gen_entries = parse_cst(gen_toks);

    std::vector<std::shared_ptr<Generic>> generics;

    for (auto ge : gen_entries) {
        auto gen_name = pope(ge, TT_identifier).value;

        auto gen = Generic::get(std::make_unique<Name>(gen_name));
        if (ge.size()) {
            pope(ge, TT_colon);
            auto constraint = parse_type(ge);
            gen->constraint = constraint;
        }

        generics.push_back(gen);
    }

    return generics;
}

TypeVec parse_type_args(std::vector<Token>& toks, bool is_fish) {
    auto ta_block = is_fish ? read_block(toks, IND_generics) : read_block(toks, IND_angles);
    auto entries = parse_cst(ta_block);

    TypeVec type_args;
    for (auto e : entries) {
        auto ty = parse_type(e);
        type_args.push_back(ty);
    }
    return type_args;
}

_Type parse_type(std::vector<Token>& toks, Container* container) {
    enum WrapType {
        PTR,
        REF,
        SLICE,
        ARR,
    };
    struct Wrap {
        WrapType type;
        std::unique_ptr<ConstantValue> val;
        Wrap(WrapType type) { this->type = type; }
        Wrap(WrapType type, std::unique_ptr<ConstantValue> c) {
            this->type = type;
            this->val = std::move(c);
        }
    };

    std::vector<Wrap> wraps;
    while (true) {
        if (toks[0].is(TT_bitwise_and)) {
            popf(toks);
            wraps.push_back(REF);
        } else if (toks[0].is(TT_star)) {
            popf(toks);
            wraps.push_back(PTR);
        } else if (toks[0].is(TT_l_square_bracket)) {
            popf(toks);
            if (toks[0].is(TT_r_square_bracket)) {
                popf(toks);
                wraps.push_back(SLICE);
            } else {
                auto len = parse_const(popf(toks));
                pope(toks, TT_r_square_bracket);
                wraps.push_back({ARR, std::move(len)});
            }
        } else
            break;
    }

    auto wrapify = [&wraps](_Type type) -> _Type {
        _Type ret = type;

        for (auto wrap = wraps.rbegin(); wrap < wraps.rend(); wrap++) {
            switch (wrap->type) {
            case PTR:
                ret = Ptr::get(ret);
                break;
            case REF:
                ret = ReferenceType::get(ret);
                break;
            case SLICE:
                ret = DynListType::get(ret);
                break;
            case ARR:
                auto size = dynamic_cast<Integer*>(wrap->val.get());
                if (!size)
                    except(E_BAD_ARRAY_LEN, "Array lengths are expected to be constant integers");
                ret = ListType::get(ret, std::move(*(std::unique_ptr<Integer>*)&wrap->val));
                break;
            }
        }

        return ret;
    };

    auto first = popf(toks);
    if (first.is(TT_string))
        return wrapify(DynListType::get(Primitive::get(PR_uint8)));
    if (first.isTypeTok())
        return wrapify(Primitive::get(primitive_mappings[first.type]));
    if (first.is(TT_identifier)) {
        pushf(toks, first);
        auto name = parse_long_name(toks);
        if (toks[0].is(TT_lesser)) {
            auto args = parse_type_args(toks, false);
            return wrapify(ParameterizedTypeRef::get(TypeRef::get(std::move(name)), args));
        } else
            return wrapify(TypeRef::get(std::move(name)));
    } else if (first.is(TT_struct)) {
        if (!container)
            except(E_BAD_TYPE, "Struct types may only be declared as members of a container");
        auto struct_content = read_block(toks, IND_braces);
        if (struct_content.size()) {
            expects(struct_content[struct_content.size() - 1], TT_semicolon);
            struct_content.pop_back();
        }

        auto entries = parse_tst(struct_content, TT_semicolon);

        std::map<std::string, _Type> fields;
        std::vector<std::string> field_order;
        for (auto entry : entries) {
            auto field_name = pope(entry, TT_identifier).value;
            pope(entry, TT_colon);
            auto field_type = parse_type(entry);
            field_order.push_back(field_name);
            fields[field_name] = field_type;
        }
        return wrapify(StructType::get(fields, field_order, container));
    } else if (first.is(TT_enum)) {
        if (!container)
            except(E_BAD_TYPE, "Enum types may only be declared as members of a container");
        auto enum_content = read_block(toks, IND_braces);
        if (enum_content.size()) {
            expects(enum_content[enum_content.size() - 1], TT_semicolon);
            enum_content.pop_back();
        }

        auto entries = parse_tst(enum_content, TT_semicolon);
        std::vector<std::string> members;
        for (auto e : entries) {
            if (e.size() != 1 || !e[0].is(TT_identifier)) {
                print_toks(e);
                except(E_BAD_TYPE, "An Enum member may only contain a single identifier");
            };
            members.push_back(e[0].value);
        }
        return wrapify(EnumType::get(members, container));

    } else if (first.is(TT_l_paren)) {
        pushf(toks, first);
        auto tuple_content = read_block(toks, IND_parens);
        auto tuple_members = parse_cst(tuple_content);
        TypeVec tv;
        for (auto m : tuple_members) {
            auto type = parse_type(m);
            tv.push_back(type);
        }
        return wrapify(TupleType::get(tv));
    } else
        except(E_BAD_TYPE, "Failed to parse type: " + first.value);
}
